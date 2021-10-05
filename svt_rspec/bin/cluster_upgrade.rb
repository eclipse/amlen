#!/bin/ruby

"""
upgrade messagesight cluster.
"""

require 'logger'
require 'imahelpers'
require 'knife_zero_driver'
require 'cluster_status'



module IMAServerClusterUpgrade

#define accessor that can be r/w outside of module
attr_accessor :standalone

@options = {}

"""
Use following knife args:

/home/tester/drivers/knife-zero-driver.rb --converge --scenario_file=/home/tester/scenario01_metadata.json --user=tester --runlist \"recipe[imaserver-docker::clean],recipe[imaserver-docker::create]\" --debug --parallel --logfile=/tmp/create

"""
@knifezero_args=[]
@cluster_status_args=[]
#declare pids array for force parallel deployment
@pids=[]

DOCKER_UPGRADE="recipe[imaserver-docker::clean],recipe[imaserver-docker::create]"
DOCKER_STOP="recipe[imaserver-docker::stop]"
DOCKER_START="recipe[imaserver-docker::start]"

RPM_UPGRADE="recipe[imaserver-rpm::upgrade],recipe[imaserver-rpm::start]"
RPM_STOP="recipe[imaserver-rpm::stop]"
RPM_START="recipe[imaserver-rpm::start]"

ACCEPT_LICENSE="recipe[svtperf-base::accept-license]"

@upgrade_runlist = ""
@stop_runlist = ""
@start_runlist = ""
#logger to use for debug
@logto

#are we being invoked on cmd line or included
@standalone=false

  def IMAServerClusterUpgrade.process_args(args=nil)
    print "IMAServerClusterUpgrade: Processing args...\n"

    #initialize some options
    @options[:nostop]=false
    @options[:dc_roles]=false
    @options[:debug]=false
    @options[:force]=false
    @options[:accept_lic]=false

    #options = OpenStruct.new
    #aparser = OptionParser.new
    if args
      print "\n\n IMAServerClusterUpgrade: Not using global ARGV ... using args passed in\n"

      args.each do |opt|
        print("\nopt type:#{opt.inspect}\n")
        case opt
          when /--scenario_file=(.*)/
             @options[:filename]=$1
          when /--user=(.*)/
             @options[:username]=$1
          when /--logfile=(.*)/
             @options[:logfile]=$1
          when /--nostop/
             @options[:nostop]=true
          when /--acceptLic/
             @options[:accept_lic]=true
          when /--dc_roles/
             @options[:dc_roles]=true
          when /--force/
             @options[:force]=true
          when /--force_parallel/
             @options[:force_p]=true
          when /--install_type=(.*)/
             @options[:type]=$1
          when /--repo_path=(.*)/
             @options[:repo_path]=$1
          else 
             #we must be the log hash
               @options[:debug]=true
               print("\n\nin else opt type: #{opt.inspect}\n\n")
               @logto=opt['--debug']
        end
      end

    else
      #were a standalone app. parse args passed in

      OptionParser.new do |opt|
        opt.on('-s','--scenario_file filename','SVTPVT scenario file') { |o| @options[:filename] = o}
        opt.on('-u','--user username','Username for ssh used by knife') { |o| @options[:username] = o }
        opt.on('-d','--debug','Turn on debug output') { |o| @options[:debug] = true }
        opt.on('-o','--order_ssp','Upgrade order as Standalones,Standbys then Primaries. Otherwise upgrade Standalones then each HA pair (Default)') { |o| @options[:ssp] = true }
        opt.on('-r','--dc_roles','Dont care state of server roles after upgrade. Otherwise keep original roles before upgrade (Default)') { |o| @options[:dc_roles] = true }
        opt.on('-L','--logfile logfile','output knife debug to <nodename_pid>') { |o| @options[:logfile] = o }
        opt.on('-S','--status','Output status of cluster before upgrade.') { |o| @options[:status] = true }
        opt.on('-f','--force','Do not check messagesight server status for upgrade. Simply run upgrade runlist against each cluster node in serial order') { |o| @options[:force] = true }
        opt.on('-n','--nostop','Do not stop rolling upgrade on failure. Continue to next node or pair depending on upgrade order') { |o| @options[:nostop]=true}
        opt.on('-p','--force_parallel','Same as force but upgrade all nodes in parallel') { |o| @options[:force_p] = true }
        opt.on('-t','--install_type type','[rpm|docker] type of server install') { |o| @options[:type] = o }

        opt.on('-C','--repo_path repopath','Chef repoistory path to cookbooks and nodes') { |o| @options[:repo_path] = o }
        opt.on('-A','--acceptLic','After upgrade accept license as \"Production\"') { |o| @options[:accept_lic] = true }
      end.parse!
    end
    if @options[:filename].nil?
      print "\n --scenario_file is required\n"
      exit(1)
    end

    #setup runlists according to install type
    if @options[:type].nil?
      print "\n --install_type (or -t) must be specified. [rpm|docker] currently supported\n"
      exit(1)
    elsif @options[:type] != "rpm" && @options[:type] != "docker"
      print "\n --install_type (or -t) must be [rpm|docker]. Passed in: #{@options[:type]}\n"
      exit(1)
    elsif @options[:type] == "rpm"
      if @options[:accept_lic] && !@options[:force_p] && !@options[:force]
        #include accept license recipe
        @upgrade_runlist = [RPM_UPGRADE,ACCEPT_LICENSE].join(',')
      else
        @upgrade_runlist = RPM_UPGRADE
      end

      @stop_runlist = RPM_STOP
      @start_runlist = RPM_START
    elsif @options[:type] == "docker"
      if @options[:accept_lic] && !@options[:force_p] && !@options[:force]
        #include accept license recipe
        @upgrade_runlist = [DOCKER_UPGRADE,ACCEPT_LICENSE].join(',')
      else
        @upgrade_runlist = DOCKER_UPGRADE
      end

      @stop_runlist = DOCKER_STOP
      @start_runlist = DOCKER_START
    end

    if @options[:repo_path]
      ENV['REPO_PATH']=@options[:repo_path]
    end

    #setup logger if running as standalone
    if @standalone
      @logto=Logger.new($stdout)
      if @options[:debug]
        @logto.level=Logger::DEBUG
      else
        @logto.level=Logger::INFO
      end
    end

    if @options[:force] && @options[:force_p]
      print "\n specify --force or --force_parallel not both\n"
      exit(1)
    end
  end

  def IMAServerClusterUpgrade.poll_state(node,status,role=nil)
    sleep_timeout = 300
    sleep_interval = 10
    reconn_interval = 10
    
    total_sleep=0

    @logto.debug "\nStart poll_state...\n"

    #loop until node reaches
    begin
      #sleep before polling
      sleep sleep_interval
      total_sleep += sleep_interval


      #in cases of a server start taking a long time, connections will fail for service requests
      #lets retry status if server is not fully up yet
      begin
        node_state=IMAServer::Helpers::Service.status(node['node_host'],9089)
      rescue Errno::ECONNREFUSED
        sleep reconn_interval
        total_sleep +=reconn_interval
        if total_sleep > sleep_timeout
          print "\nError: node: #{node['node_host']} refused network connection ... node in bad state\n"
          @logto.debug "\nEnd poll_state...return false\n"
      

          return false
        else
          print "\nWarning: retrying to retrive status for #{node['node_host']}\n"
          retry
        end
      end
      
      
      
      print "cluster_upgrade: poll_state: polling node for status: #{status} role: #{role}\n"
      print "cluster_upgrade: poll_state: current status: --->\n #{node_state}\n<---\n"
      
      hash_state=JSON.parse(node_state)
      
      ha_state=hash_state["HighAvailability"]
      server_status=hash_state["Server"]
      

      @logto.debug "cluster: debug: polling server_status: #{server_status["StateDescription"]} match: #{server_status["StateDescription"].include?(status)} role_status: #{ha_state["NewRole"]} match: #{ha_state["Role"] == role}\n"


      if total_sleep >= sleep_timeout
        print "cluster_upgrade: We have timeout waiting on server to become status: #{status} role: #{role}\n"
        @logto.debug "\nEnd poll_state...return false\n"


        return false
      end

    end until (unless role and status then ((ha_state["NewRole"] == role) && (server_status["StateDescription"].include?(status))) else (server_status["StateDescription"].include?(status))end)
    
    @logto.debug "\nEnd poll_state...return true\n"

    #we reached state returne true
    return true
  end

  def IMAServerClusterUpgrade.non_primary(node,standby=false)

    @logto.debug "\nStart non-primary upgrade...\n"

    #upgrade non-primary (ie standalone or standby or unknown state)
    pid=KnifeZeroDriver.converge(node['node_name'],node['node_host'],nil,[@stop_runlist,@upgrade_runlist].join(','))

    if @options[:force_p]
      @pids.push(pid)
    end

    if (standby && !poll_state(node,"Standby","STANDBY"))
      print "Error: non_primary: #{node['node_host']} failed to become standby ... skiping upgrade on pair: #{node}\n"
      @logto.debug "\nEnd non-primary upgrade...return false\n"
      return false
      #@TODO we should also check standalone status in else statement
    end

    if @options[:force_p]
      print "\nupgrade force deployed successfully in parallel with pid: #{pid}\n"
    else
      print "-------------------------------\n"
      print "|upgrade non-primary: Success!|\n"
      print "-------------------------------\n"
    end
    
    return true
  end

  def IMAServerClusterUpgrade.primary(pair)

    @logto.debug "\nStart primary upgrade...\n"

    #first stop primary and wait for standby to become primary
    KnifeZeroDriver.converge(pair[0]['node_name'],pair[0]['node_host'],nil,@stop_runlist)

    #poll until paired standby becomes new primary
    if (!poll_state(pair[1],"Running (production)","PRIMARY"))
      print "Error: standby: #{pair[1]['node_host']} failed to become primary ... skiping upgrade on pair: #{pair}\n"
      @logto.debug "\nEnd primary upgrade... return false\n"

      return false
    else
        #primary in stop state already. Lets upgrade!
        KnifeZeroDriver.converge(pair[0]['node_name'],pair[0]['node_host'],nil,@upgrade_runlist)

        #after upgrade primary should now be new standby, lets check
        if (!poll_state(pair[0],"Standby","STANDBY"))
          print "Warning: primary: #{pair[0]} upgraded, but not in expected state\n"
          if @options[:debug]
            @logto.debug "\nEnd primary upgrade... return false\n"
          end

          return false
        else
          print "primary: Successful upgrade! primary: #{pair[0]} upgraded and now in Standby state\n"
          print "---------------------------\n"
          print "|upgrade primary: Success!|\n"
          print "---------------------------\n"

          return true
        end
    end
  end

  """
   switch roles of a HA pair
  """

  def IMAServerClusterUpgrade.switch_roles(pair)


    @logto.debug "\nStart switch roles...\n"


    #first find which one is primary and standby
    standby=nil
    primary=nil

    pair.each do |anode|
      begin
        node_state=IMAServer::Helpers::Service.status(anode['node_host'],9089)
      rescue StandardError => e
        print "\n\nError: exception occured retriving status for #{anode['node_host']}\n"
        print "\n#{e.backtrace}"
        print "\nCannot switch roles returning\n"
    
        @logto.debug "\nEnd switch roles... return false\n"
    

        return false
      end

      hash_state=JSON.parse(node_state)
        #check state of server. only update if server is standby or has HA disabled
      ha_state=hash_state["HighAvailability"]
      
      print "\n\ncluster_upgrade: switch current HA config: #{ha_state}\n"
      print "\n\ncluster_upgrade: switch current HA enabled: #{ha_state["Enabled"]}\n"
      
      
      if (ha_state["Enabled"] && ha_state["NewRole"] == "STANDBY")
        print "\n\nFound standby: #{anode['node_name']}\n"
        standby=anode
      else (ha_state["Enabled"] && ha_state["NewRole"] == "PRIMARY")
        print "\n\nFound primary: #{anode['node_name']}\n"
        primary=anode
      end
    end

    """
     switch roles by restarting primary, check standby becomes primary, restart old primary to becomes new standby
    """

    #first stop primary and wait for standby to become primary
    KnifeZeroDriver.converge(primary['node_name'],primary['node_host'],nil,@stop_runlist)

    standby_failed=false
    if (!poll_state(standby,"Running (production)","PRIMARY"))
        print "Error: standby: #{standby['node_host']} failed to become primary\n"
        standby_failed=true
    end


    if standby_failed && @options[:nostop]
      print "restarting primary during switch roles\n"
      KnifeZeroDriver.converge(primary['node_name'],primary['node_host'],nil,@start_runlist)

      print "Error: switch_roles: restarting primary during switch roles due to standby not responding\n"

      if (!poll_state(primary,"Running (production)","PRIMARY"))
        print "Error: switch_roles: #{primary['node_host']} failed to become primary\n"
        print "Error: switch_roles: HA down!! during switching of roles pair: #{pair}\n"
      else
        print "switch_roles: Primary back up! but standby is down :(\n"
      end

      #either way we failed, return false

      @logto.debug "\nEnd switch roles... return false\n"


      return false

    elsif standby_failed
      print "\nError: failing switching roles for primary: #{primary['node_host']} standby: #{standby['node_host']} \n"

      @logto.debug "\nEnd switch roles... return false\n"


      return false
    else
      print "restarting primary during switch roles\n"
      KnifeZeroDriver.converge(primary['node_name'],primary['node_host'],nil,@start_runlist)

      if (!poll_state(primary,"Standby","STANDBY")) 
        print "Error: switch_roles: #{primary['node_host']} failed to become standby\n"
        print "Error: switch_roles: HA down!! during switching of roles pair: #{pair}\n"

        @logto.debug "\nEnd switch roles... return false\n"


        return false
      else
        print "------------------------\n"
        print "|switch_roles: Success!|\n"
        print "------------------------\n"
        return true
      end

    end
    
  end

  """
  Upgrade standbys then primaries
  """
  def IMAServerClusterUpgrade.order_ssp(standalones,pairs)

    #upgrade all standalones
    standalones.each do |anode|
      print "\nUpgrading standalone: #{anode['node_name']}\n"
      success=non_primary(anode)

      if !success && !@options[:nostop]
        print "\nUpgrade of standalone failed ... returning\n"
        return false
      end
    end

    #upgrade all standbys of pair
    pairs.each do |apair|
      print "\nUpgrading standby: #{apair[1]}\n"
      success=non_primary(apair[1],true)

      if !success && !@options[:nostop]
        print "\nUpgrade of pair failed ... returning\n"
        return false
      end

    end


    #upgrade primary of pair
    pairs.each do |apair|
      print "\nUpgrading primary: #{apair[0]}\n"
      success=primary(apair[0])

      #switch roles after primary upgrade
      if success && !@options[:dc_roles]
        if !switch_roles(apair) && !@options[:nostop]
          print "\n Error:Switch roles failed... returning\n"
          return false
        end
      elsif !success && !@options[:nostop]
        print "\nUpgrade of pair failed ... returning\n"
        return false
      end


    end

    #no failures!
    return true


  end

  """
  upgrade one pair at a time
  """
  def IMAServerClusterUpgrade.order_pairs(standalones,pairs)

    #upgrade standalones
    standalones.each do |anode|
      print "\nUpgrading standalone: #{anode['node_name']}\n"
      success=non_primary(anode)

      if !success && !@options[:nostop]
        print "\nUpgrade of standalone... returning\n"
        return false
      end

    end
    
    #upgrade each pair at one time
    pairs.each do |apair|
      print "\nUpgrading HA pair standby: #{apair[1]['node_host']} primary: #{apair[0]['node_host']}\n"
      #upgrade standby first
      success=non_primary(apair[1],true)

      if !success && !@options[:nostop]
        print "\nUpgrade of standby in pair failed ... returning\n"
        return false
      end

      if (!poll_state(apair[1],"Standby","STANDBY"))
        print "Error: standby: #{apair[1]['node_host']} failed to become standby ... skiping upgrade on pair: #{apair}\n"
        if not @options[:nostop]
          return false
        else next
        end
      end

      print "\nUpgrading HA pair primary: #{apair[0]['node_host']} with upgraded standby: #{apair[1]['node_host']}\n"
      #upgrade primary. pass both nodes since state of standby changes
      success=primary(apair)
      if !success && !@options[:nostop]
        print "\nUpgrade of primary in pair failed ... returning\n"
        return false
      end

      #switch roles after primary upgrade. 
      if success && !@options[:dc_roles]
        if !switch_roles(apair) && !@options[:nostop]
          print "\n Error:Switch roles failed... returning\n"
          return false
        end
      elsif !success && !@options[:nostop]
        print "\nUpgrade of pair failed ... returning\n"
        return false
      end
    end #end upgrade pairs loop

    #no failures? then return true
    return true
  end

  """
  upgrade entire cluster of nodes
  """
  def IMAServerClusterUpgrade.process_upgrade(nodes)

    #First, find standby's,primaries  or standalones; and create pairs 

    standbys=[]
    primarys=[]
    standalones=[]
    node_pairs=[]

    #check force upgrade if true ignore state of cluster nodes and just upgrade (eazy pezy)
    if @options[:force] || @options[:force_p]


      #upgrade all nodes
      nodes.each do |anode|
          print "\nUpgrading unknown state node: #{anode['node_host']}\n"
          non_primary(anode)
      end
      
      if @options[:force_p]
        # non_primary executing knifezerodriver.converge should had pushed all pids. Need to wait for each pid to 
        #finished
        print "\n\nWaiting on forked knives!!\n"
      
        pid_rc=[]
        afailure=false
        @pids.each do |apid|
            Process.waitpid(apid)
            pid_rc.insert($?.exitstatus)
            afailure=true if $?.exitstatus !=0
            print "\n\n Process #{apid} returned"
        end
      
        #return if failure  
        return false if afailure
      end

      print "\n-------------------------\n"
      print "|force upgrade: Success!|\n"
      print "-------------------------\n"

      #were done
      return true
      
    end

    #otherwise retrive state of all nodes and build out  (long run)
    print("logto type: #{logto.inspect}\n\n")
    @logto.debug "\n Going to send status request to each node of cluster\n"


    nodes.each do |anode|
        print "\n\nRetriving status ... Node name: #{anode['node_name']} Node host: #{anode['node_host']}\n"
        #converge existing nodes using supplied runlist
        begin
          node_state=IMAServer::Helpers::Service.status(anode['node_host'],9089)
        rescue StandardError => e
          print "\n\nError: exception occured retriving status for #{anode['node_host']}\n"
          
          if !@options[:nostop]
            print "\n --nostop option turned off...aborting upgrade for entier cluster!!!\n"
            print "\n#{e.backtrace}"
            return false
          else
            print "\n --nostop option provided ... Continue to next node\n"
            next
          end          
        end

        print "\n\ncluster_upgrade: imaserver current status: #{node_state}\n"

        hash_state=JSON.parse(node_state)
        #check state of server. only update if server is standby or has HA disabled
        ha_state=hash_state["HighAvailability"]
        server_state=hash_state["Server"]

        print "\n\ncluster_upgrade: imaserver current HA config: #{ha_state}\n"
        print "\n\ncluster_upgrade: imaserver current HA enabled: #{ha_state["Enabled"]}\n"

        #attached ha state and server state to anode. We'll determine if HA pair should
        # be upgraded depending on ha and server state when we pair up below
        anode['ha_state']=ha_state
        anode['server_state']=server_state

        if (ha_state["Enabled"] == true && ha_state["NewRole"] == "STANDBY" && server_state["StateDescription"].include?("Standby"))
          print "\n\nFound standby: #{anode['node_name']}\n"
          standbys.push(anode)
        elsif  (ha_state["Enabled"] == false && server_state["StateDescription"].include?("Running (production)"))
          print "\n\nFound standalone: #{anode['node_name']}\n"
          standalones.push(anode)
        elsif (ha_state["Enabled"] == true && ha_state["NewRole"] == "PRIMARY" && server_state["StateDescription"].include?("Running (production)"))
          print "\n\nFound primary: #{anode['node_name']}\n"
          primarys.push(anode)
        else
          print "\nNode: #{anode['node_name']} is in unknown bad state ... skipping upgrade"
        end

    end


    #find standby partner for each primary and insert into pairs array. Also
    #check to make sure primary and standby are in correct state. Otherwise do not upgrade
    # the ha pair
    primarys.each do |anode|
      print "\nFind matching standby for primary: #{anode['node_host']}\n"
      found=false
      standbys.each do |snode|
        if snode['attrs'].fetch('imaserver')['ha_group'] == anode['attrs'].fetch('imaserver')['ha_group']
          found=true
          print "\nFound HA partner: #{snode['node_name']} group: #{snode['attrs'].fetch('imaserver')['ha_group']} for primary: #{anode['node_host']} group: #{snode['attrs'].fetch('imaserver')['ha_group']}\n"


          primary_hastate=anode['ha_state']
          standby_hastate=snode['ha_state']

          #check state of pair before adding to node pairs array. Only add if syncnodes is 2 for both
          #standbys and primaries
          if primary_hastate["SyncNodes"]==2 && standby_hastate["SyncNodes"] == 2 

            #create add partner to pairs
            print "\nCreating HA pair primary: #{anode['node_host']} standby: #{snode['node_host']}\n"
            node_pairs.push([anode,snode])
          else
            print "\nWarning: primary: #{anode['node_host']} and its partner standby: #{snode['node_host']} are not in acceptable state for upgrade ... skipping upgrade for this pair\n\n"
          end

        end
      end

      if !found
        print "\nWarning:Standby partner not found for primary: ---> #{anode}<---\n\n\nWill not upgrade this primary node\n"

        if @options[:nostop]
          print "\nIgnoring warning of HA pair state ... Continuing to next HA pair\n"
        else
          print "\nWill not continue upgrade of these cluster HA pairs...stopping upgrade process immediately!!!\n"
          return false
        end

        next
      end
    end

    #time to upgrade cluster
    if standalones.empty? && node_pairs.empty?
      print "\n\nWarning:No standalones or HA pairs are upgradeable ... were done!!\n"
      print "\nSuggestion:Use --force option to bypass state check of nodes\n\n"
    elsif @options[:ssp]
      print "\n\ncluster_upgrade: upgrading using order_ssp\n\n"
      success=order_ssp(standalones,node_pairs)
    else
      print "\n\ncluster_upgrade: upgrading each HA pair\n\n"
      success=order_pairs(standalones,node_pairs)
    end
        
    if success then return true
    else return false 
    end

  end
    

  def IMAServerClusterUpgrade.run


    #process knife zero driver args
    if @options[:debug]
      #add debug to knife driver
      #pass in our logto instance
      @knifezero_args.push(['--debug']=>@logto)
    end
    if @options[:user]
      @knifezero_args.push("--user=#{@options[:user]}")
    end
    if @options[:logfile]
      @knifezero_args.push("--logfile=#{@options[:logfile]}")
    end
    if @options[:filename]
      @knifezero_args.push("--scenario_file=#{@options[:filename]}")
    end
    if @options[:force_p]
      @knifezero_args.push("--parallel")
    end

    KnifeZeroDriver.process_args(@knifezero_args)      

    if @options[:status]
      @cluster_status_args.push("--scenario_file=#{@options[:filename]}")
      # DEBUG cluster status turned off. Its too much and really should
      # invoke cluster_status as standalone for debugging if issues arise. Or
      # remove comments below. But expect alot of debug info.
      #if @options[:debug]
      #  @cluster_status_args.push("--debug"=>@logto)
      #end
      IMAServerClusterStatus.process_args(@cluster_status_args)
    end



    if @options[:status]
      IMAServerClusterStatus.process_metadata
      IMAServerClusterStatus.retrive_status
      IMAServerClusterStatus.output
    end

    success=process_upgrade(KnifeZeroDriver.process_metadata(@options[:filename]))

    if @options[:status]
      IMAServerClusterStatus.process_metadata
      IMAServerClusterStatus.retrive_status
      IMAServerClusterStatus.output
    end

    print <<DONE
 --------------
| Upgrade done |
 --------------
DONE
    return success ? 0:1
  end
end #end IMAServerClusterUpgrade module

include IMAServerClusterUpgrade

#check if we are running standalone
if __FILE__ == $0
  IMAServerClusterUpgrade::standalone=true
  IMAServerClusterUpgrade.process_args
  IMAServerClusterUpgrade.run
end
