#!/bin/ruby
"""
  printout status of each node defined in metadata file

  messagesight status current format is :

 { \"Version\":\"v1\",
  \"Server\":{ \"Name\":\"0PvZDuhH\",\"State\":10,\"StateDescription\":\"Status = Standby\",\"ErrorCode \":0,\"UpTimeSeconds\":277,\"UpTimeDescription\":\"0 days 0 hours 4 minutes 37 seconds\",\"Version\":\"V.next Beta 20151015-2034\",\"ErrorMessage\":\"\"},  \"Container\": {},  \"HighAvailability\": {\"Role\":\"STANDBY\",\"IsHAEnabled\":1,\"NewRole\":\"STANDBY\",\"OldRole\":\"UNSYNC\",\"ActiveNodes\":2,\"SyncNodes\":2,\"PrimaryLastTime\":\"\",\"PctSyncCompletion\":-1,\"ReasonCode\":0,\"ReasonString\":\"\"},  \"Cluster\": {\"Status\": \"Initializing\", \"Name\": \"purple01-svtperf\", \"ConnectedServers\": 0, \"DisconnectedServers\": 0},  \"Plugin\": {\"Status\": \"Stopped\"},  \"MQBridge\": {\"Status\": \"Stopped\" }}


  cluster_status --scenario_file <filename>
"""

require 'logger'
require 'imahelpers'
require 'knife_zero_driver'

#need to include the imahelpers namespace
include IMAServer

module IMAServerClusterStatus

@options = {}
@nodes = []
@nodes_status = [['node_host'=>'','admin_up'=>'','build_version'=>'','conn_servers'=>'','disconn_servers'=>'','role'=>'','active_nodes'=>'','sync_nodes'=>''],]

#logger to use for debug
@logto

#are we being invoked on cmd line or included
@standalone=false

  def IMAServerClusterStatus.process_args(args=nil)
    print "IMAServerClusterStatus: Processing args...\n"

    if args
      print "\n\n IMAServerClusterStatus: Not using global ARGV ... using args passed in: #{args}\n"

      args.each do |opt|
        case opt
        when /--scenario_file=(.*)/
          @options[:filename]=$1
        else 
          #we must be the log hash. set logger to passed in value
          @options[:debug]=true
          @logto=opt['--debug']
          print("\n\nin else opt type: #{@logto.inspect}\n\n")
        end
      end

    else    

      OptionParser.new do |opt|
        opt.on('-s','--scenario_file filename','SVTPVT scenario file') { |o| @options[:filename] = o}
        opt.on('-d','--debug','Turn on debug output') { |o| @options[:debug] = true }
      end.parse!
    end

    #setup logger if running as standalone
    if @standalone or @logto.nil?
      @logto=Logger.new($stdout)
      if @options[:debug]
        @logto.level=Logger::DEBUG
      else
        @logto.level=Logger::INFO
      end
    end
    
    #setup imahelpers logger
    IMAServer::logto=@logto

  end

  def IMAServerClusterStatus.retrive_status(nodes=nil)

    if nodes
      input_nodes=nodes
    else
      input_nodes=@nodes
    end

    input_nodes.each_with_index do |anode,i|


      @nodes_status[i]={'node_host' => anode['node_host']}

      node_down=false
      begin
        node_state=IMAServer::Helpers::Service.status(anode['node_host'],9089)
      rescue StandardError => e
        @logto.debug "\n\nError: exception occured retriving status for #{anode['node_host']}\n"
        @logto.debug "\n#{e.backtrace}"
        @logto.debug "\nCannot connect to node returning...\n"
        node_down=true
      end

      if !node_down
        @nodes_status[i].merge!({'admin_up'=>'UP',})

        hash_state=JSON.parse(node_state)
        #check state of server. only update if server is standby or has HA disabled
        ha_state=hash_state["HighAvailability"]
        server_state=hash_state["Server"]
        cluster_state=hash_state["Cluster"]


        @logto.debug "\n\ncluster_status: HA config: #{ha_state}\n"
        @logto.debug "\n\ncluster_status: server state: #{server_state}\n"
        @logto.debug "\n\ncluster_status: cluster state: #{cluster_state}\n"


        @nodes_status[i].merge!({ 'status' => server_state["StateDescription"],
                                  'build_version' =>server_state["Version"],
                                  'role' => ha_state["NewRole"],
                                  'sync_nodes' => ha_state["SyncNodes"].to_s,
                                  'active_nodes' =>ha_state["ActiveNodes"].to_s,
                                  'conn_servers' => cluster_state["ConnectedServers"].to_s,
                                  'disconn_server' => cluster_state["DisconnectedServers"].to_s})



      else
        @nodes_status[i].merge!({'admin_up'=>'DOWN',
                          'status' => 'UNKNOWN',
                          'conn_servers' => 'UNKNOWN',
                          'disconn_server' => 'UNKNOWN',
                          'role' => 'UNKNOWN',
                          'active_nodes' =>'UNKNOWN',
                          'sync_nodes' => 'UNKNOWN',
                          'build_version' =>'UNKNOWN',})

      end

    end
  end

  def IMAServerClusterStatus.output(status=nil)

    if status
      input_nodes=status
    else
      input_nodes=@nodes_status
    end

    
    #insert first row with titles
    table=[['Node_Host','Admin_Status','Server_Status','Version','HARole','HA_SyncNodes','HA_ActiveNodes','Cluster_Connected_Servers','Cluster_Disconnected_Server'],]

    input_nodes.each do |anode,line=[]|

      @logto.debug "\n node line: #{anode}\n"


      anode.each_pair do |k,v|
      
        @logto.debug "\n created key: #{k} value: #{v}\n"
      

        
        if v 
          #replace spaces with _ in value
          #new_v=v.sub(' ','_')
          line.push(v)
        else
          line.push('NOVALUE')
        end
      end


      @logto.debug "\n created line: #{line}\n"


      table.push(line)

    end


    @logto.debug "\n created table: #{table}\n"


    # Calculate widths
    widths = []
    table.each do |row|
      c = 0
      row.each do |col|
        widths[c] = (widths[c] && widths[c] > col.length) ? widths[c] : col.length
        c += 1
      end
    end
    # Indent the last column left.
    last = widths.pop()
    format = widths.collect{|n| "%-#{n}s"}.join(" ")
    format += " %-#{last}s\n"
    # @Logto.Debug each line.
    print "\n\n\n\n-----------------------------------------------------------------------------------Cluster Status---------------------------------------------------------------------------------------------\n"
    table.each do |row|

      @logto.debug "\n print with format: #{format} line: #{row}\n"

       
      printf(format,*row)
    end
    print "\n--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"

  end

  def IMAServerClusterStatus.process_metadata(input_file=nil)
    if input_file

      @logto.debug "Process metadata file from input arg: #{input_file}\n"

      file = input_file
    else

      @logto.debug "Process metadata file: #{@options[:filename]}\n"

      file = @options[:filename]
    end

    array_args=[]
    #index of real nodes found in file
    n=0 
    File.readlines(file).each_with_index do |line,x|


      @logto.debug "Processing line #{line}\n"


      #skip comment lines
      if line[0] != '#'
        array_args = line.split(' ')


        @logto.debug "#{array_args}\n"

        
        #first string should be node name
        @nodes[n]={'node_name'=>array_args[0],
          #second is ip or hostname
          'node_host'=>array_args[1],
          #rest is messagesight specific attrs
          'attrs' => {'imaserver'=> {'cluster_group' => array_args[2],'cluster_discovery_port'=> array_args[3].to_i,'cluster_control_port' => array_args[4].to_i,'cluster_msg_port' => array_args[5].to_i,'ha_group' => array_args[6],'pref_primary'=> ((array_args[7] =~ (/(true|t|yes|y|1)$/i)) == 0),
        }}}


        @logto.debug "\nNode name: #{@nodes[n]['node_name']} Node host: #{@nodes[n]['node_host']} Server attrs: #{@nodes[n]['imaserver']}\n"


        n+=1
      end
    end

    #return build nodes for includers
    return @nodes
  end

  def IMAServerClusterStatus.run
    process_metadata
    retrive_status
    output
  end
end

include IMAServerClusterStatus

#check if we are running standalone
if __FILE__ == $0
  IMAServerClusterStatus::standalone=true
  IMAServerClusterStatus.process_args
  IMAServerClusterStatus.run
end
