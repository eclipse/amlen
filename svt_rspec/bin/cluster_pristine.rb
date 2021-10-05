#!/bin/ruby

"""
pristine install messagesight cluster nodes.
"""

require 'chef'
require 'chef/log'
require 'drivers/imaserver_helpers'
require 'drivers/knife_zero_driver'
require 'drivers/cluster_status'



module IMAServerClusterPristine

@options = {}

"""
Use following knife args:

/home/tester/drivers/knife-zero-driver.rb --converge --scenario_file=/home/tester/scenario01_metadata.json --user=tester --runlist \"recipe[imaserver-docker::cleanall],recipe[imaserver-docker::create]\" --debug --parallel --logfile=/tmp/create

"""
@knifezero_args=[]
@cluster_status_args=[]

@pristine_runlist = "recipe[imaserver-docker::cleanall],recipe[imaserver-docker::create]"
#@stop_runlist = "recipe[imaserver-docker::stop]"
#@start_runlist = "recipe[imaserver-docker::start]"

  def IMAServerClusterPristine.process_args(args=nil)
    print "IMAServerClusterPristine: Processing args...\n"

    #initialize some options
    @options[:debug]=false
    @options[:parallel]=false
    @options[:force]=false

    #options = OpenStruct.new
    #aparser = OptionParser.new
    if args
      print "\n\n IMAServerClusterPristine: Not using global ARGV ... using args passed in\n"

      args.each do |opt|
        case opt
          when /--scenario_file=(.*)/
             @options[:filename]=$1
          when /--user=(.*)/
             @options[:username]=$1
          when /--logfile=(.*)/
             @options[:logfile]=$1
          when /--debug/
             @options[:debug]=true
          when /--parallel/
             @options[:parallel]=true
        end
      end

    end


    OptionParser.new do |opt|
      opt.on('-s','--scenario_file filename','SVTPVT scenario file') { |o| @options[:filename] = o}
      opt.on('-u','--user username','Username for ssh used by knife') { |o| @options[:username] = o }
      opt.on('-d','--debug','Turn on debug output') { |o| @options[:debug] = true }
      opt.on('-L','--logfile logfile','output knife debug to <nodename_pid>') { |o| @options[:logfile] = o }
      opt.on('-S','--status','Output status of cluster before upgrade.') { |o| @options[:status] = true }
      opt.on('-p','--parallel','Do not stop rolling upgrade on failure. Continue to next node or pair depending on upgrade order') { |o| @options[:parallel]=true}

    end.parse!

    if @options[:filename].nil?
      print "\n --scenario_file is required\n"
      exit(1)
    end

  end

  def IMAServerClusterPristine.process_pristine(nodes)

    pids=[]

    nodes.each do |anode|
      #pristine install a node
      print "\nPristine installing node:#{anode['node_host']}\n"
      pid=KnifeZeroDriver.converge(anode['node_name'],anode['node_host'],nil,@pristine_runlist)

      if @options[:parallel]
        pids.push(pid)
      end
    end

    if @options[:parallel]
      print "\n\nWaiting on forked knives!!\n"
      pids.each do |apid|
        Process.waitpid(apid)
        print "\n\n Process #{apid} returned"
      end
    end


  end

  def IMAServerClusterPristine.run
    process_args

    #proces knife zero module args
    if @options[:debug]
      #add debug to knife driver
      @knifezero_args.push('--debug')
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
    if @options[:parallel]
      @knifezero_args.push("--parallel")
    end

    KnifeZeroDriver.process_args(@knifezero_args)
    
    #process cluster status args
    if @options[:status]
      @cluster_status_args.push("--scenario_file=#{@options[:filename]}")
      if @options[:debug]
        @cluster_status_args.push("--debug")
      end
      IMAServerClusterStatus.process_args(@cluster_status_args)
    end



    if @options[:status]
      IMAServerClusterStatus.process_metadata
      IMAServerClusterStatus.retrive_status
      IMAServerClusterStatus.output
    end

    process_pristine(KnifeZeroDriver.process_metadata(@options[:filename]))

    if @options[:status]
      IMAServerClusterStatus.process_metadata
      IMAServerClusterStatus.retrive_status
      IMAServerClusterStatus.output
    end


  end
end #end IMAServerClusterPristine module

#check if we are running standalone
if __FILE__ == $0
  IMAServerClusterPristine.run
end
