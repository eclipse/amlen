require 'IMAServerState'
require 'cluster_upgrade'

class IMAClusterState 
  attr_accessor :imaservers,:nodes,:logto,:upgrade_args,:chef_repo,:log_path

  #mixin cluster upgrade module
  include IMAServerClusterUpgrade

  def initialize(node_file,chef_repo,log_path)

    #initialize nodes to array
    @nodes=[]
    @imaservers=[]
    @logto=IMALOGGER

    @chef_repo=chef_repo
    @log_path=log_path

    @upgrade_args=["--scenario_file=#{node_file}",'--user=tester',{'--debug'=>@logto},'--install_type=rpm',"--repo_path=#{chef_repo}","--logfile=#{log_path}/upgrade_","--acceptLic"]
    #export repo path as env var to be picked up
    #in ~/.chef/knife.rb. This is kinda of hacky for chef zero setup
    ENV['CHEF_REPO']=chef_repo

    #parse node file
    process_metadata(node_file)
    @logto.debug("IMACluster: initialize: going to init imaservers array\n\n")
    @nodes.each_with_index do |anode,i|
      @imaservers[i]=IMAServerState.new(anode['node_host'],anode['node_name'])
    end
  end

  def process_metadata(input_file=nil)
    if input_file
      @logto.debug("Process metadata file from input arg: #{input_file}\n")
      file = input_file
    end

    array_args=[]
    #index of real nodes found in file
    n=0 
    File.readlines(file).each_with_index do |line,x|

      @logto.debug "Processing line: <beg>#{line}<end>\n"

      #skip comment lines
      if line[0] != '#' && line.strip !~ /^\s*$/

        array_args = line.strip.split(' ')

        @logto.debug "line_args: #{array_args}\n"
              
        #first string should be node name
        @nodes.push({'node_name'=>array_args[0],
          'node_host'=>array_args[1],
        })

        @logto.debug "\nNode name: #{@nodes[n]['node_name']} Node host: #{@nodes[n]['node_host']}\n"

        n+=1
      end
    end
  end

  def each
    @imaservers.each do |aserver|
      yield aserver
    end
  end


  def upgrade
    IMAServerClusterUpgrade.process_args(@upgrade_args)
    rc= IMAServerClusterUpgrade.run 
    if rc == 0
      #retrive new state of imaservers
      @logto.debug("\nupgrade: setting up new imaserverstate's\n")
      @imaservers.clear #reset,clear out array of servers
      @nodes.each_with_index do |anode,i|
        @imaservers[i]=IMAServerState.new(anode['node_host'],anode['node_name'])
        @logto.debug("\n imaserver in  loop: #{i} buildid: #{imaservers[i].build_id}\n\n")
      end

    end
    @logto.debug("\nupgrade: returning rc: #{rc}\n")
    return rc
  end
  
end
