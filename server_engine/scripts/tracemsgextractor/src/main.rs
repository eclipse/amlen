#[macro_use] extern crate lazy_static;
//#[macro use] extern crate clap;

use std::io::{self, prelude::*, BufReader};
use std::fs::{self, File};
use std::path::PathBuf;
use regex::Regex;
use std::collections::HashMap;
use math;
use json;
use alphanumeric_sort;
use std::process::Command;
use clap::Clap;
use chrono::{DateTime,FixedOffset};

struct ConnectState {
    pub clientid: String,
    pub connected: bool,
    pub last_change_file: String,
    pub last_change_line: u64,
    pub last_change_time: Option<DateTime<FixedOffset>> //We use this in the devicestate_hash but not subclients_hash
}

struct PublishData {
    pub receiving_clientid: String,
    pub date: String,
    pub time: String
}

#[derive(Clap)]
//#[clap(version = "0.1.0", author = "Ananyous")]
struct Opts {
    /// Sets a directory to find trace in.
    #[clap(short, long)]
    tracedir: String
}

fn get_sub_from_clientid(clientid: &str) -> String {
    let first_ascii = &clientid[0..1]; //Expecting A - if its not ascii - won't match anywat
    
    if first_ascii != "A" {
        panic!("Clientid {} can't have a shared sub", clientid);
    }
    
    let clientparts: Vec<&str> = clientid.split(':').collect();
    
    let mut sub = String::from(clientparts[1]);
    sub.push_str(":");
    sub.push_str(clientparts[2]);
    
    sub
}

fn ensure_devicestate_hash( devicestate_hash: &mut HashMap<String, HashMap<String,ConnectState>>
                          , clientid: &str) {                      
    let sub = get_sub_from_clientid(clientid);

    if let Some(_devicehash) = devicestate_hash.get(&sub) {
        //No action needed
    } else {
        let devicehash: HashMap<String,ConnectState> = HashMap::new();
        
        devicestate_hash.insert(sub, devicehash);
    }               
}

fn compare_devicestates( devicestate_hash: &HashMap<String, HashMap<String,ConnectState>>
                       , recsub1: &str, recsub2: &str)
{
    let mut most_devs = 0;
    let mut most_dev_sub = "";
    let mut most_dev_subhash: Option<&HashMap<String,ConnectState>> = None;
    
    let mut another_sub = "";
    let mut another_subhash: Option<&HashMap<String,ConnectState>> = None;
    
    if let Some(devhash1) = devicestate_hash.get(recsub1) {
        most_devs         = devhash1.len();
        most_dev_sub      = recsub1;
        most_dev_subhash  = Some(devhash1);
    } else {
        println!("No device state data found for {}. Can't compare to {}",
                  recsub1, recsub2);
        return;
    }
    
    if let Some(devhash2) = devicestate_hash.get(recsub2) {
        if most_devs >= devhash2.len() {
            another_sub       = recsub2;
            another_subhash   = Some(devhash2);
        } else {
            another_sub       = most_dev_sub;
            another_subhash   = most_dev_subhash;
            
            most_devs         = devhash2.len();
            most_dev_sub      = recsub2;
            most_dev_subhash  = Some(devhash2);
        }
    } else {
        println!("No device state data found for {}. Can't compare to {}",
                  recsub2, recsub1);
        return;
    }
    
    println!("Looking at each device in {} and comparing it to state according to {}",
                            most_dev_sub, another_sub);

    //Checked we have hashes for both - can safely unwrap.
    let most_dev_subhash = most_dev_subhash.unwrap();
    let another_subhash  = another_subhash.unwrap();
    
    let mut num_same       = 0;
    let mut num_different  = 0;
    let mut num_notpresent = 0;
    
    for (client, connstate) in most_dev_subhash {
        if let Some(otherconn) = another_subhash.get(client) {
            if connstate.connected == otherconn.connected {
                num_same += 1;
            } else {
                num_different += 1;
                
                println!("Mismatch for device {}. {} set on {}:{}, {} set on {}:{}",
                             &connstate.clientid,
                             &most_dev_sub, &connstate.last_change_file, &connstate.last_change_line,
                             &another_sub, &otherconn.last_change_file,  &otherconn.last_change_line);
            }
        } else {
            num_notpresent += 1;
        }
    }
    
    println!("{} devices matched. {} devices different. {} devices only in {}",
                num_same, num_different, num_notpresent, most_dev_sub);
}

fn analyse_devicestate_hash( devicestate_hash: HashMap<String, HashMap<String,ConnectState>>) {
    println!("There are {} different subscriptions we've tracked", devicestate_hash.len());
    

    
    for (sub, hash) in &devicestate_hash {
        let hashlen = hash.len();
        
        println!("subscription: {} has tracked online/offline state for {} devices", 
                              sub, hashlen);
    }
    
    compare_devicestates(&devicestate_hash, "msy76z:appconnectsrv", "msy76z:qaconnectsrv");
}

fn update_devicestate_hash( devicestate_hash: &mut HashMap<String, HashMap<String,ConnectState>>
                          , receiving_clientid: &str
                          , json_data: json::JsonValue
                          , filepath: &PathBuf
                          , line_num: u64 ) {
                          
    let closecode_int = json_data["CloseCode"].to_string().parse::<i32>().unwrap();
    let action        = json_data["Action"].to_string();
    let time          = DateTime::parse_from_rfc3339(&json_data["Time"].to_string()).unwrap();
    let mut do_insert = true;
    let sub = get_sub_from_clientid(receiving_clientid);
            
    if action != "Connect" &&  action != "Disconnect" && action != "FailedConnect" {
       //do_insert = false;
       panic!("Action that we've not coded to cope with: {} (on {}:{}", 
                  action, filepath.as_path().display().to_string(),line_num);
    }
     
    if let Some(devicehash) = devicestate_hash.get_mut(&sub) {
        //Get existing entry and compare date/time - only process the event if it's newer
        
        let clientid = json_data["ClientID"].to_string();
        
        if let Some(prev_entry) = devicehash.get(&clientid) {
            if prev_entry.last_change_time.unwrap() > time {
                //We are processing events out of order - we have an older one -ignore
                println!("MILDLY INTERESTING: Subclient {} received events for {} out of order. {}:{} was newer than {}:{}",
                            receiving_clientid, clientid,
                            &prev_entry.last_change_file, prev_entry.last_change_line,
                            filepath.as_path().display().to_string(),line_num);
                do_insert = false;
            }
        }
        
        let mut connected = true;
        
        //288 is takeover - so a connection is ending as another one arrives - still one is connected
        // so we treat a disconnect(288) as a sign of continued connection
        if action == "Disconnect" && closecode_int != 288 {
            connected = false;           
        } else if action != "Connect" && action != "Disconnect" {
            println!("Skipping insert due to action of {}", action);
            do_insert = false;
        }

        if do_insert {
            devicehash.insert( clientid.clone()
                             , ConnectState{
                                   clientid: clientid,
                                   connected: connected,
                                   last_change_file: filepath.as_path().display().to_string(),
                                   last_change_line: line_num,
                                   last_change_time: Some(time)});
        }
    } else {
        panic!("Processing messages for sub {} but haven't parsed a valid connect for a client connecting to it");
    }
}

fn parse_trace_file( subclients_hash: &mut HashMap<String, ConnectState>
                   , devicestate_hash: &mut HashMap<String, HashMap<String,ConnectState>>
                   , filepath: &PathBuf
                   , parse_errors: &mut u64) -> std::io::Result<()> {
    lazy_static! {
        static ref CONNECT_REGEX: Regex = Regex::new(r"([^T]*)T([^Z]*)Z.*User is authenticated and authorized.*connect=([^\s]*)\s*client=([^\s]*)\s").unwrap();
        static ref DISCONNECT_REGEX: Regex = Regex::new(r"([^T]*)T([^Z]*)Z.*Closing virtual connection \(CWLNA1111\): connect=([^\s]*)\s.*name=([^\s]*)\s.*RC=([^\s]*)\s").unwrap();
        static ref PUBSTART_REGEX: Regex = Regex::new(r"([^T]*)T([^Z]*)Z ([^\s]*) .*MQTT send.*PUBLISH connect=(\d*): len=(\d*)[\s$]").unwrap();
        static ref MSGCONTENTS_REGEX: Regex = Regex::new(r"[^\[]*\[(.*)\][\s$]*").unwrap();
    }
    println!("About to parse {}", filepath.as_path().display().to_string());
    let file = File::open(&filepath)?;
    let reader = BufReader::new(file);
  
    let mut count: u64 = 1;
    
    let mut num_msg_lines: i64 = 0; //0 = not in msg, 5= 5 more lines of message expected
    let mut msg_sofar = String::from("");
    
    let mut pubdata: Option<PublishData> = None;

    for lineresult in reader.lines() {
        if let Ok(line) = lineresult {
            //println!("{}", line);        
            //if (count % 20000) == 0 {
            //    println!("Processing line {}", count); 
            //}
        
            count += 1;
        
            //If we are currently in the middle of reading a message from the trace
            if num_msg_lines > 0 {
               if let Some(caps) = MSGCONTENTS_REGEX.captures(&line) {
                   let msgpart = caps.get(1).map_or("PARSE ERROR", |m| m.as_str());
                   
                   msg_sofar.push_str(msgpart);
               } else {
                   panic!("message interleaved with other trace lines - need more complex parser");
               }
               num_msg_lines -= 1;
               
               if num_msg_lines == 0 {
                   //Got entire message
                   let json_start = msg_sofar.find('{').unwrap();
                   let json_slice = &msg_sofar[json_start..];
                   //println!("json = {}", json_slice);
                   
                   let parsed = json::parse(json_slice).unwrap();
                   //println!("json,parsed = {}", json::stringify(parsed));
                   let msgpubdata = pubdata.unwrap();
                   
                   println!("{} {} subclient {} received publish (clientid: {}, Action: {}, closecode: {})",
                                       msgpubdata.date, msgpubdata.time, msgpubdata.receiving_clientid,
                                       parsed["ClientID"], parsed["Action"], parsed["CloseCode"]);
                                       
                   update_devicestate_hash( devicestate_hash
                                          , &msgpubdata.receiving_clientid
                                          , parsed
                                          , filepath
                                          , count);

                   msg_sofar = String::from("");
                   pubdata = None;
               }
            } else if let Some(caps) = CONNECT_REGEX.captures(&line) {
                //println!("Found a Connect.");
                //println!("Date    = {}", caps.get(1).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Time    = {}", caps.get(2).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Connect = {}", caps.get(3).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Client  = {}", caps.get(4).map_or("PARSE ERROR", |m| m.as_str()));
                
                let parseddate = caps.get(1).map_or("PARSE ERROR", |m| m.as_str());
                let parsedtime = caps.get(2).map_or("PARSE ERROR", |m| m.as_str());
                let connect  = caps.get(3).map_or("PARSE ERROR", |m| m.as_str());
                let clientid = caps.get(4).map_or("PARSE ERROR", |m| m.as_str());
                
                println!("{} {} Subclient {} connected (changed conn state). connect={}",
                           parseddate, parsedtime, clientid,connect);
                
                subclients_hash.insert(String::from(connect), ConnectState {
                    clientid: String::from(clientid),
                    connected: true,
                    last_change_file: filepath.as_path().display().to_string(),
                    last_change_line: count,
                    last_change_time: None, //We don't use the time in the subclients_hash
                });
                
                ensure_devicestate_hash(devicestate_hash, &clientid);
                
            } else if let Some(caps) = DISCONNECT_REGEX.captures(&line) {
                //There will be LOTS of disconnects for clients we are not tracking, we only care
                //if it is one we are tracking 
                //println!("Found a Disconnect.");
                //println!("Date    = {}", caps.get(1).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Time    = {}", caps.get(2).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Connect = {}", caps.get(3).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Client  = {}", caps.get(4).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("RC      = {}", caps.get(5).map_or("PARSE ERROR", |m| m.as_str()));
                
                let parseddate = caps.get(1).map_or("PARSE ERROR", |m| m.as_str());
                let parsedtime = caps.get(2).map_or("PARSE ERROR", |m| m.as_str());
                let connect    = caps.get(3).map_or("PARSE ERROR", |m| m.as_str());
                let client     = caps.get(4).map_or("PARSE ERROR", |m| m.as_str());
                let rc         = caps.get(5).map_or("PARSE ERROR", |m| m.as_str());
                
                let mut do_insert: bool = false;

                if let Some(connectdata) = subclients_hash.get(connect) {
                    //This is a connection we are tracing!
                    
                    if client != connectdata.clientid {
                        panic!("Connection {} started with id {} on {}:{} and ended with id {} on {}:{}",
                                 connect, &connectdata.clientid, 
                                 &connectdata.last_change_file, &connectdata.last_change_line,
                                 client, filepath.as_path().display().to_string(), count);
                    }
                    println!("{} {} Subclient {} disconnected (changed conn state). connect={} rc={}",
                           parseddate, parsedtime, connectdata.clientid,connect, rc);
                    
                    do_insert=true;
                }
                
                if do_insert {
                    subclients_hash.insert(String::from(connect), ConnectState {
                        clientid: String::from(client),
                        connected: false,
                        last_change_file: filepath.as_path().display().to_string(),
                        last_change_line: count,
                        last_change_time: None, //We don't use the time in the subclients_hash
                    });
                }
            
            } else if let Some(caps) = PUBSTART_REGEX.captures(&line) {
                //println!("Date    = {}", caps.get(1).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Time    = {}", caps.get(2).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Thread = {}", caps.get(3).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("Connect  = {}", caps.get(4).map_or("PARSE ERROR", |m| m.as_str()));
                //println!("len  = {}", caps.get(5).map_or("PARSE ERROR", |m| m.as_str()));
                
                let parseddate = caps.get(1).map_or("PARSE ERROR", |m| m.as_str());
                let parsedtime = caps.get(2).map_or("PARSE ERROR", |m| m.as_str());
                let connect    = caps.get(4).map_or("PARSE ERROR", |m| m.as_str());
                
                if let Some(connectdata) = subclients_hash.get(connect) {
                    pubdata = Some( PublishData{
                        receiving_clientid: connectdata.clientid.clone(),
                        date: String::from(parseddate),
                        time: String::from(parsedtime),
                    });
                    
                    let len_str = caps.get(5).map_or("PARSE ERROR", |m| m.as_str());
                    let num_len = len_str.parse::<f64>().unwrap();
                    num_msg_lines = math::round::ceil(num_len / 32.0, 0) as i64;
                //println!("len  = {}, num_msg_lines={}", num_len, num_msg_lines);
                } else {
                    //It's a connection we were tracing before the earliest trace file
                    println!("ERROR: Failed to find matching connect for connection {} seen on {}:{}",
                                  connect,  filepath.as_path().display().to_string(), count);
                    *parse_errors += 1;
                }
            }
        }
    }
    println!("Completed parse of {}", filepath.as_path().display().to_string());

    Ok(())
}

fn get_sorted_tracefiles(rootdir: &str) -> io::Result<Vec<PathBuf>> {
    let mut tracefilelist: Vec<PathBuf> = Vec::new();
    let mut imatracefile: Option<PathBuf> = None;
    
    for entry in fs::read_dir(rootdir)? {
        let entry = entry?;
        let path = entry.path();
        
        if !path.is_dir() {
            //println!("Found file: {:?}", path);
            let fname = path.as_path().display().to_string();
            if let Some(_foundlog) = fname.find(".log") {
                if let Some(_found) = fname.find("imatrace.log") {
                    if let Some(_foundalready) = imatracefile {
                        panic!("Found two imatrace.log files - can't order!");
                    }
                    imatracefile = Some(path);
                } else {
                    tracefilelist.push(path);
                }
            } else {
                println!("Ignoring file {} as it doesn't look like a log file", fname);
            }
        }
    }
    //imatrace.log is special - no timestamp in name but always most recent...
    
    alphanumeric_sort::sort_path_slice(&mut tracefilelist);
    
    if let Some(tracepath) = imatracefile {
        tracefilelist.push(tracepath);
    }
    
    for entry in &tracefilelist {
        println!("Found file: {}", entry.as_path().display().to_string());
    }    
    Ok(tracefilelist)
}

fn main() {
    let opts: Opts = Opts::parse();

    let file_list = get_sorted_tracefiles(&opts.tracedir)
                       .expect("Failed to get sorted list of trace files");
    
    let mut subclients_hash: HashMap<String, ConnectState> = HashMap::new();
    let mut devicestate_hash: HashMap<String, HashMap<String,ConnectState>> = HashMap::new();
    
    let mut parse_errors: u64 = 0;
    
    for file in &file_list {
        let mut need_extract = false;
        
        if let Some(ext) = file.extension() {
            if "gz" == ext {
                need_extract = true;
            }
        }
        
        if need_extract {
            let gzfname = file.as_path().display().to_string();
            let extractedfilename = &gzfname[..gzfname.len() - String::from(".gz").len()];

            if file_list.contains(&PathBuf::from(&extractedfilename)) {
                println!("Skipping extracting {} as {} is also in list",gzfname, extractedfilename);        
            } else {
                println!("Found file that needs extracting : {}",gzfname);
                
                let _output = Command::new("gunzip")
                                .arg("-k")
                                .arg(&gzfname)
                                .output()
                                .expect(&format!("failed to unzip {}", &gzfname));

                let result = parse_trace_file( &mut subclients_hash
                                             , &mut devicestate_hash
                                             , &PathBuf::from(extractedfilename)
                                             , &mut parse_errors);
    
                if let Err(err_data) = result {
                    panic!("Failed to parse extracted file {} with {}", extractedfilename, err_data);
                }
                
                let _output = Command::new("rm")
                                .arg(&extractedfilename)
                                .output()
                                .expect(&format!("failed to rm file we extracted {}", &extractedfilename));

                
            }
        } else {
            let result = parse_trace_file( &mut subclients_hash
                                         , &mut devicestate_hash
                                         , &file
                                         , &mut parse_errors);
    
            if let Err(err_data) = result {
                panic!("Failed to parse {} with {}", file.as_path().display().to_string(), err_data);
            }
        }
    }
    
    analyse_devicestate_hash(devicestate_hash);
    
    if parse_errors > 0 {
        println!("Parse errors: {} - Check the logs for ERROR", parse_errors);
    } else {
        println!("All good.");
    }
}
