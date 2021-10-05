require 'json'
require 'logger'
require 'imahelpers'

#include the IMAServer namespace defined in 'imahelpers'
include IMAServer

class IMAServerState
  attr_accessor :primary,:standby,:endpoints,:hubs,:certs,:cluster,:ha,:top_policies,:message_policies,
                :sec_profiles,:conn_policies,:que_policies,:sub_policies,:build_id,
                :status

  attr_accessor :chef_node,:hostname

  #define logger class variable so that inner classes can use same logger
  @@logto=IMALOGGER
  #directoy to store logs
  LOGS_PATH=ENV["LOGS_PATH"]

  #standard port for restapi
  ADMIN_PORT=9089

  #config object uri's
  ENDPOINT_URI="Endpoint"
  TOPICPOLICY_URI="TopicPolicy"
  HUB_URI="MessageHub"
  SUBSCRIPTIONPOLICY_URI="SubscriptionPolicy"
  SECURITYPROFILE_URI="SecurityProfile"
  CONNECTIONPOLICY_URI="ConnectionPolicy"
  MESSAGEPOLICY_URI=""
  QUEUEPOLICY_URI="QueuePolicy"

  class Hub
    attr_accessor :name,:description
    
    def initialize(hub_obj)
      IMAServerState.logto.debug("creating Hub object\n")
      # ["DemoHub", {"Description"=>"Demo Message Hub."}]
      @name=hub_obj[0]
      @description=hub_obj[1]["Description"]

    end
  end

  class Endpoint
    attr_accessor :description,:name,:sec_profile,:msg_hub,:max_sendsz,:max_msgsz,:enable,:batch_msgs,:port,:protocol,:interface,:interface_name,:conn_policies,:topic_policies,:queue_policies,:subscription_policies

    def initialize(ep_obj)

      #["DemoEndpoint",{"Port"=>16102, "Enabled"=>false, "Protocol"=>"All", "Interface"=>"All", "InterfaceName"=>"All", "ConnectionPolicies"=>"DemoConnectionPolicy", "TopicPolicies"=>"DemoTopicPolicy", "SubscriptionPolicies"=>"DemoSubscriptionPolicy", "MaxMessageSize"=>"4096KB", "MessageHub"=>"DemoHub", "EnableAbout"=>true, "Description"=>"Unsecured endpoint for demonstration use only. By default, both JMS and MQTT protocols are accepted."}]

      @name=ep_obj[0]
      @description=ep_obj[1]["Description"]
    end
  end

  class TopicPolicy
    attr_accessor :description, :name , :topic, :cert_common_names, :client_id, :group_id, :client_addr, :user_id, :protocol, :max_msgttl, :max_msgs, :action_list, :max_msgsbehave, :disconn_clientnotify
    def initialize(tp_obj)

      #"TopicMessagingPolicy": { "MaxMessageTimeToLive": "unlimited","ClientID": "","MaxMessages": 25000,"DisconnectedClientNotification": false,"CommonNames": "","Description": "Topic Messaging Policy - Performance","GroupID": "","Topic": "*","Protocol": "JMS,MQTT","ActionList": "Publish,Subscribe","UserID": "","MaxMessagesBehavior": "RejectNewMessages","ClientAddress": ""

      @name=tp_obj[0]
      @description=tp_obj[1]["Description"]
    end

  end

  class SecProfile
    attr_accessor :name , :tls_enable, :ltpa_profile, :oauth_profile, :min_protocol, :cert_profile, :useclient_cert, :usepasswd_auth, :ciphers, :useclient_cipher

    def initialize(secp_obj)
# "PerfSecProfile": {"TLSEnabled": true,"OAuthProfile": "","UsePasswordAuthentication": false,"LTPAProfile": "","MinimumProtocolMethod": "TLSv1.2","Ciphers": "Fast","CertificateProfile": "PerfCertProfile","UseClientCertificate": false,"UseClientCipher": false}
      @name=secp_obj[0]
      @cert_profile=secp_obj[1]["CertificateProfile"]
    end
  end

  class ConnPolicy
    attr_accessor :description,:name ,:allow_persist_msg ,:cert_common_names,:client_id,:group_id,:client_addr,:user_id,:protocol,:allow_durable

    def initialize(cp_obj)
# "AllConnectionPolicy": {"CommonNames": "","Description": "Performance All Connection Policy","AllowDurable": true,"ClientAddress": "","AllowPersistentMessages": false,"Protocol": "JMS,MQTT","UserID": "","ClientID": "","GroupID": ""}
      @name=cp_obj[0]
      @description=cp_obj[1]["Description"]
    end
  end

  class SubPolicy
    attr_accessor :description, :name , :subscription, :cert_common_names, :client_id, :group_id, :client_addr, :user_id, :protocol, :max_msgs, :action_list, :max_msgsbehave

    def initialize(sp_obj)
# "SharedSubMessagingPolicy": {"ClientID": "","MaxMessages": 25000,"CommonNames": "","Description": "Shared Subscription Messaging Policy - Performance","ActionList": "Receive,Control","UserID": "","Subscription": "*","Protocol": "JMS,MQTT","GroupID": "","MaxMessagesBehavior": "RejectNewMessages","ClientAddress": ""}
      @name=sp_obj[0]
      @description=sp_obj[1]["Description"]
      
    end

  end

  class MsgPolicy
    attr_accessor :description, :name , :cert_common_names, :client_id, :group_id, :client_addr, :user_id, :protocol, :max_msgttl, :max_msgs, :destination, :destination_type, :action_list, :max_msgsbehave, :disconn_clientnotify

  end

  class QuePolicy
    attr_accessor :description, :name , :cert_common_names, :queue, :client_id, :group_id, :client_addr, :user_id, :protocol, :max_msgttl, :action_list

    def initialize(qp_obj)
# "QueueMessagingPolicy": {"CommonNames": "","Description": "Queue Messaging Policy - Performance","Queue": "*","ActionList": "Send,Receive,Browse","ClientAddress": "","Protocol": "JMS","UserID": "","MaxMessageTimeToLive": "unlimited","ClientID": "","GroupID": ""}
      @name=qp_obj[0]
      @description=qp_obj[1]["Description"]

    end
  end

  def initialize(host,node)

    @primary=false
    @standby=false
    @build_id=nil
    @hubs=[]
    @endpoints=[]
    @certs=[]
    @cluster=nil
    @ha=nil
    @top_policies=[]
    @sec_profiles=[]
    @conn_policies=[]
    @sub_policies=[]
    @message_policies=[]
    @que_policies=[]
    @hostname=host
    @chef_node=node
    @status=nil

    IMAServer::host=host
    IMAServer::port=ADMIN_PORT
    IMAServer::logto=IMALOGGER
    get_status
    get_hubs
    get_endpoints
    get_connpolices
    get_topicpolicies
    get_secprofiles
    get_quepolicies
    get_subpolicies
    @@logto.debug("\n\nimaserverstate: initialize refreshed state\n\n")
  end

  #define a class method to retrive logger for inner classes
  def IMAServerState.logto
    @@logto
  end

  def get_endpoints
    response=IMAServer::Helpers::Config.get(ENDPOINT_URI)

    @@logto.debug("recieved endpoint response #{response}")

    eps_parse=JSON.parse(response)

    eps_parse["Endpoint"].each do |aep|
      @@logto.debug("an endpoint: #{aep}")
      @endpoints.push(Endpoint.new(aep))
    end

  end

  def get_hubs
    response=IMAServer::Helpers::Config.get(HUB_URI)


    hubs_parse=JSON.parse(response)

    hubs_parse["MessageHub"].each do |ahub|
      @@logto.debug("a hub: #{ahub}")
      @hubs.push(Hub.new(ahub))
    end
  end

  def get_connpolices
    response=IMAServer::Helpers::Config.get(CONNECTIONPOLICY_URI)

    conn_parse  =JSON.parse(response)

    conn_parse["ConnectionPolicy"].each do |aconn|
      @@logto.debug("a conection policy: #{aconn}")
      @conn_policies.push(ConnPolicy.new(aconn))
    end
  end

  def get_topicpolicies
    response=IMAServer::Helpers::Config.get(TOPICPOLICY_URI)
    toppol_parse =JSON.parse(response)

    toppol_parse["TopicPolicy"].each do |atoppol|
      @@logto.debug("\na topic policy: #{atoppol}\n")
      @top_policies.push(TopicPolicy.new(atoppol))
    end

  end
  def get_secprofiles
    response=IMAServer::Helpers::Config.get(SECURITYPROFILE_URI)
    secpro_parse=JSON.parse(response)

    secpro_parse["SecurityProfile"].each do |asecpro|
      @@logto.debug("\nsecurity profile: #{asecpro}")
      @sec_profiles.push(SecProfile.new(asecpro))
    end

  end
  def get_quepolicies
    response=IMAServer::Helpers::Config.get(QUEUEPOLICY_URI)
    quepol_parse  =JSON.parse(response)

    quepol_parse["QueuePolicy"].each do |aquepol|
      @@logto.debug("\na queue policy: #{aquepol}")
      @que_policies.push(QuePolicy.new(aquepol))
    end

  end

  def get_subpolicies
    response=IMAServer::Helpers::Config.get(SUBSCRIPTIONPOLICY_URI)

    subpol_parse=JSON.parse(response)

    subpol_parse["SubscriptionPolicy"].each do |asubpol|
      @@logto.debug("a subscription policy: #{asubpol}")
      @sub_policies.push(SubPolicy.new(asubpol))
      end

  end

  def get_status
    node_state=IMAServer::Helpers::Service.status

    hash_state=JSON.parse(node_state)
    #check state of server. only update if server is standby or has HA disabled
    ha_state=hash_state["HighAvailability"]
    server_state=hash_state["Server"]
    cluster_state=hash_state["Cluster"]

    @@logto.debug("\nserver state received hash: #{server_state}\n")
    @@logto.debug("\ncluster state received hash: #{cluster_state}\n")
    @@logto.debug("\nha state received hash: #{ha_state}\n")
    @build_id=server_state["Version"]
    @status=server_state["StateDescription"]

    case ha_state["Role"]
    when "PRIMARY"
      @primary=true
      @standby=false
    when "STANDBY"
      @primary=false
      @standby=true
    end
  end
end
