require 'spec_helper'
require 'IMAClusterState'
require 'logger'

NODES_FILE='/home/tester/tests/upgrade-clusterHA.nodes'
CHEF_REPO='/home/tester/tests/imacluster/svtperf-autotests-repo'
BUILD_ID=ENV["BUILD_ID"]
LOGS_PATH=ENV["LOGS_PATH"]

#check current state of cluster before upgrade
describe IMAClusterState do

  
  subject(:clusterstate){IMAClusterState.new(NODES_FILE,CHEF_REPO,LOGS_PATH)}

  describe "a 4 node imaserver cluster before upgrading to build: #{BUILD_ID}" do
    it "should contain same config before upgrade on each node" do
      clusterstate.each do |aimaserver|
        
        expect(aimaserver.hubs.find { |hub| hub.name == "PerfHub"}).to_not be_nil
        
        
        expect(aimaserver.endpoints.find { |endpoint| endpoint.name == "PerfSecureEndpoint_4"}).to_not be_nil
        
        expect(aimaserver.endpoints.find { |endpoint| endpoint.name == "PerfSecureEndpoint_3"}).to_not be_nil
        
        expect(aimaserver.endpoints.find { |endpoint| endpoint.name == "PerfNonSecureEndpoint_2"}).to_not be_nil
                
        expect(aimaserver.endpoints.find { |endpoint| endpoint.name == "PerfNonSecureEndpoint_1"}).to_not be_nil

        expect(aimaserver.top_policies.find { |top_pol| top_pol.name == "TopicMessagingPolicy"}).to_not be_nil

        expect(aimaserver.sec_profiles.find { |sec_pro| sec_pro.name == "PerfSecProfile"}).to_not be_nil

        expect(aimaserver.conn_policies.find { |con_pol| con_pol.name == "AllConnectionPolicy"}).to_not be_nil

        expect(aimaserver.sub_policies.find { |sub_pol| sub_pol.name == "SharedSubMessagingPolicy"}).to_not be_nil
        expect(aimaserver.que_policies.find { |que_pol| que_pol.name == "QueueMessagingPolicy"}).to_not be_nil
        
      end      
    end

  end

  describe "a cluster upgrade" do
  it "should succeed without errors" do
      expect(clusterstate.upgrade).to be(0)
    end
  end

  describe "a 4 node upgraded cluster" do

    it "should have new upgraded version on each node and same config before upgrade" do
      clusterstate.imaservers.each do |aimaserver|
        
        expect(aimaserver.build_id).to include(BUILD_ID)
        
        expect(aimaserver.hubs.find { |hub| hub.name == "PerfHub"}).to_not be_nil
        
        expect(aimaserver.endpoints.find { |endpoint| endpoint.name == "PerfSecureEndpoint_4"}).to_not be_nil
        
        expect(aimaserver.endpoints.find { |endpoint| endpoint.name == "PerfSecureEndpoint_3"}).to_not be_nil
        expect(aimaserver.endpoints.find { |endpoint| endpoint.name == "PerfNonSecureEndpoint_2"}).to_not be_nil
        
        expect(aimaserver.endpoints.find { |endpoint| endpoint.name == "PerfNonSecureEndpoint_1"}).to_not be_nil
        expect(aimaserver.top_policies.find { |top_pol| top_pol.name == "TopicMessagingPolicy"}).to_not be_nil

        expect(aimaserver.sec_profiles.find { |sec_pro| sec_pro.name == "PerfSecProfile"}).to_not be_nil

        expect(aimaserver.conn_policies.find { |con_pol| con_pol.name == "AllConnectionPolicy"}).to_not be_nil

        expect(aimaserver.sub_policies.find { |sub_pol| sub_pol.name == "SharedSubMessagingPolicy"}).to_not be_nil
        expect(aimaserver.que_policies.find { |que_pol| que_pol.name == "QueueMessagingPolicy"}).to_not be_nil

      end
    end
  end

end

