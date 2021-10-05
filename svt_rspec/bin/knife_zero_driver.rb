#!/bin/ruby

"""
Script to drive knife zero command until chef-provisioning works

knife_zero_driver --runlist \"<list of recipes or roles\" --scenario_file=<svtpvt_metadata_file> [--bootstrap|--converge] [--user=<username for ssh> [--debug] [--node_status] [--help]
 

example knife bootstrap:

knife zero bootstrap george-svtpvt-purple01-imaserver01A.priv -z -x root -N purple01-imaserver01A --sudo --run-list role[imaserver],role[mswebui] -j '{\"imaserver\":{\"cluster_group\":\"purple01-svtperf\",\"cluster_discovery_port\":7104,\"cluster_control_port\":10101,\"cluster_msg_port\":10102,\"ha_group\":\"purple01_ha\"}}'


example knife converge:

 knife zero converge 'name:purple01-imaserver01A' -o \"recipe[mswebui-docker::cleanall],recipe[imaserver-docker::cleanall],recipe[imaserver-docker::create],recipe[mswebui-docker::create]\"  -x tester --sudo -VV 
"""

require 'optparse'
require 'ostruct'
require 'json'
require 'logger'

module KnifeZeroDriver

ROOT_CMD = '/usr/local/bin/knife zero'

#number of columns in metadata file supported
METADATA_COLS=8
#define accessor that can be r/w outside of module
attr_accessor :standalone

@options = {}
#initialize new node structure
# node_name: name of node to create
# node_host: network hostname or ip to communicate with node
# imaserver: specific messagesight key value used by recipes

@new_nodes=[{'node_name' => '','node_host' => '','imaserver'=>'',},]

#are we being invoked on cmd line or included
@standalone=false


  def KnifeZeroDriver.process_args(args=nil)
    print "KnifeZeroDriver: Processing args...\n"

    #options = OpenStruct.new
    #aparser = OptionParser.new
    if args
      print "\n\n KnifeZeroDriver: Not using global ARGV ... using args passed in: #{args}\n"
      args.each do |opt|
        case opt
          when /--user=(.*)/
             @options[:username]=$1
          when /--logfile=(.*)/
             @options[:logfile]=$1
          when /--parallel/
             @options[:parallel]=true
          when /--scenario_file=(.*)/
             @options[:filename]=$1
          when /--bootstrap/
             @options[:bootstrap]=true
          when /--converge/
             @options[:converge]=true
          when /--runlist=(.*)/
             @options[:run_list]=$1
          when /--parallel/
             @options[:parallel]=true
          when /--status/
             @options[:status]=true
          else 
             #we must be the log hash
             #TODOprobably should inspect to make sure its a log hash.
             @options[:debug]=true
             @logto=opt['--debug']

        end
      end
    else
    
      OptionParser.new do |opt|
        opt.on('-r','--runlist runlist','Runlist to execute "recipe[cookbook::recipe],role[arole]"') { |o| @options[:run_list] = o }
        opt.on('-b','--bootstrap','Bootstrap nodes') { |o| @options[:bootstrap] = true }
        opt.on('-s','--scenario_file filename','SVTPVT scenario file') { |o| @options[:filename] = o}
        opt.on('-c','--converge','Converge existing nodes') { |o| @options[:converge]=true }
        opt.on('-u','--user username','Username for ssh used by knife') { |o| @options[:username] = o }
        opt.on('-d','--debug','Turn on debug output') { |o| @options[:debug] = true }
        opt.on('-p','--parallel','Process each knife command in parallel') { |o| @options[:parallel] = true }
        opt.on('-L','--logfile logfile','output knife debug to <nodename_pid>') { |o| @options[:logfile] = o }
        opt.on('-S','--status','Output table state of all nodes before exit') { |o| @options[:status] = true }
        
      end.parse!
    end
    if @options[:bootstrap] && @options[:converge]
      print "Specify bootstrap or converge action"
      exit(1)
    end

    if @standalone
      @logto=Logger.new($stdout)
      if @options[:debug]
        @logto.level=Logger::DEBUG
      else
        @logto.level=Logger::INFO
      end
    end

  end

  def KnifeZeroDriver.run_cmd(cmd=nil,args=nil)


    if @options[:parallel]
      print "\n\nForking recvd cmd: #{cmd} with args: #{args} Starting process\n"

      pid = fork { exec (cmd+' '+args) }
      return pid
    else
      print "\n\nRecvd cmd: #{cmd} with args: #{args} Starting process\n"
      return (system cmd+ ' '+args) ? 0 : 1
    end
  end


  def KnifeZeroDriver.bootstrap(name,host,attrs,runlist=nil)
    #build knife zero args for bootstrap
    #add --yes to force delete
    args = 'bootstrap ' +  host +' -N '+ name + ' --yes  -z -j ' +"\'#{attrs.to_json}\'"

    if @options[:username]
      args += ' --sudo -x ' + @options[:username]
    end

    if @options[:debug]
      args += ' -VV '
    end

    #since we are module check if runlist wuz passed in, used it first
    if runlist
      args += ' --run-list ' + "\"#{runlist}\""
    elsif @options[:run_list]
      args += ' --run-list ' + "\"#{@options[:run_list]}\""
    else
      print "\n\nError: runlist required \n\n"
      exit(1)
    end

    if @options[:logfile]
      #add timestamp to make log file uniq
      args += ' 2>&1 > ' + @options[:logfile] + '_' + name + '-' + Time.now.strftime("%s")
    end

    return run_cmd(ROOT_CMD,args)
  end

  def KnifeZeroDriver.converge(name,host,attrs=nil,runlist=nil)


    args = "converge 'name:#{name}'"

    # can we use -j with attrs during converge? not sure if it updates attrs within nodes json, but
    # something to look into as a @TODO. For now ignore attrs input
    if @options[:username]
      args += ' -z --sudo -x ' + @options[:username]
    end

    if @options[:debug]
      args += ' -VV'
    end

    #since we are module check if runlist wuz passed in, used it first
    if runlist
      args += ' -o ' + "\"#{runlist}\""
    elsif @options[:run_list]
      args += ' -o ' + "\"#{@options[:run_list]}\""
    else
      print "\n\nError: runlist required \n\n"
      exit(1)
    end

    if @options[:logfile]
      #add timestamp to make log file unique
      args += ' 2>&1 > ' + @options[:logfile] + '_' + name + '-' + Time.now.strftime("%s")
    end

    return run_cmd(ROOT_CMD,args)
  end

  def KnifeZeroDriver.process_metadata(input_file=nil)

    if input_file
      print "Process metadata file from input arg: #{input_file}\n"
      file = input_file
    else
      print "Process metadata file: #{@options[:filename]}\n"
      file = @options[:filename]
    end

    array_args=[]
    #index of real nodes found in file
    n=0 
    File.readlines(file).each_with_index do |line,x|
      print "Processing line: #{line}\n"
      #skip comment lines and blank lines
      if line[0] != '#' || line !~ /\S/
        array_args = line.split(' ')

        if array_args.length == METADATA_COLS
          print "read metadata line: #{array_args}\n"
        
          #first string should be node name
          @new_nodes[n]={'node_name'=>array_args[0],
            #second is ip or hostname
            'node_host'=>array_args[1],
            #rest is messagesight specific attrs
            'attrs'=> {'imaserver' =>{'cluster_group' => array_args[2],'cluster_discovery_port'=> array_args[3].to_i,'cluster_control_port' => array_args[4].to_i,'cluster_msg_port' => array_args[5].to_i,'ha_group' => array_args[6],'pref_primary'=> ((array_args[7] =~ (/(true|t|yes|y|1)$/i)) == 0),
          }}}

          print "Node name: #{@new_nodes[n]['node_name']} Node host: #{@new_nodes[n]['node_host']} Server attrs: #{@new_nodes[n]['attrs'].to_json}\n"

          #increment node count
          n+=1
        end

      end
    end

    #return build nodes for includers
    return @new_nodes
  end

  def KnifeZeroDriver.run
    #Main
    process_metadata

    #save pids and pids rc into array
    pids=[]
    pid_rc=[]
    #determine if at least one failure occured
    afailure=false

    
    #if bootstrap filename is given lets  bootstrap, otherwise converge with runlist on existing nodes
    if @options[:bootstrap]
      @new_nodes.each_with_index do |anode|
        
        print "\n\nPassing into bootstrap Node name: #{anode['node_name']} Node host: #{anode['node_host']} Server attrs: #{anode['attrs']}\n"
        
        pid=bootstrap(anode['node_name'],anode['node_host'],anode['attrs'])
        
        #save pid in array
        if @options[:parallel]
          #save pids to wait on below
          pids.push(pid)
        else
          #for non-parallel pid is rc
          pid_rc.push(pid) 
          afailure=true if pid != 0
        end
      end
    elsif @options[:converge]
      @new_nodes.each do |anode|
        print "\n\nPassing into converge Node name: #{anode['node_name']} Node host: #{anode['node_host']} Server attrs: #{anode['attrs']}\n"
        #converge existing nodes using supplied runlist
        pid=converge(anode['node_name'],anode['node_host'],anode['attrs'])

        #save pid in array
        if @options[:parallel]
          #save pids to wait on below
          pids.push(pid)
        else
          #for non-parallel pid is rc
          pid_rc.push(pid) 
          afailure=true if pid != 0
        end
      end
    else
      print "Error:Must specify --bootstrap or --converge action"
      exit(1)
    end

    #wait on child process pids
    if @options[:parallel]
      print "\n\nWaiting on forked knives!!\n"

      pids.each do |apid|

        Process.waitpid(apid)
        pid_rc.push($?.exitstatus)

        afailure=true if $?.exitstatus !=0
      end
    end    

    print <<DONE
\n\n--------------------------------
\n\n Done with parallel knife_zero_driver with failure: #{afailure} in pid's rc: #{pid_rc}\n\n
--------------------------------
DONE
    return afailure ? 1:0


  end

end #end  module KnifeZeroDriver

include KnifeZeroDriver

#check if we are running standalone
if __FILE__ == $0
  KnifeZeroDriver::standalone=true
  KnifeZeroDriver.process_args
  KnifeZeroDriver.run
end
  





