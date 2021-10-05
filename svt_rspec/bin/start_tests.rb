#!/bin/ruby

require 'optparse'
require 'rspec'
require 'stringio'
require 'net/smtp'
require 'pry'

"""
start automated tests
"""
 
module StartTests

LOGS_PATH="/seed/tests"
TESTSUITE_PATH="/home/tester/tests"
SPECHELPER_FILE="#{TESTSUITE_PATH}/imacluster/spec/spec_helper.rb"
#currently add format and fullback trace args
RSPEC_ARGS=["--format","doc","-b"]

#include following paths to $LOAD_PATH
$: << "/home/tester/tests/bin"
$: << "/home/tester/tests/lib"
$: << "/home/tester/tests/imacluster/spec"
$: << "/home/tester/tests/imacluster/spec/models"

attr_accessor :options

@options={}
@email_notify_addrs=[]
#define smtp server and port
@email_smtp_server='smtp.sendgrid.net'
@email_smtp_port=587


#add spec files to run
@auto_tests=["#{TESTSUITE_PATH}/imacluster/spec/upgrade_spec.rb"]

  def StartTests.process_args(args=nil)
    #setup default options
    @options[:foreground]=false

    if args
      print "\n\n StartTests: Not using global ARGV ... using args passed in\n"
      args.each do |opt|
        case opt
        when /--build_id=(.*)/
          @options[:build_id]=$1
        when /--foreground/
          @options[:foreground]=true
        end
      end
    else
      print "\nprocessing args\n"
      OptionParser.new do |opt|
        opt.on('-b','--build_id id','Build ID to test') { |o| @options[:build_id] = o }
        opt.on('-f','--foreground','Start tests in foreground. Default start in background') {|o| @options[:foreground]=true}
      end.parse!

      if @options[:build_id].nil?
        print("Must specify build_id\n")
        exit(1)
      end

    end

    #pass build_id as env variable for spec. Reason? 'rspec' command cannot handle passing
    #of args into the spec from command. Bogus!
    #system("export BUILD_ID=#{options[:build_id]}")
    ENV["BUILD_ID"]=@options[:build_id]
    ENV["LOGS_PATH"]=LOGS_PATH+"/#{@options[:build_id]}"
    #setup ruby lib paths

  end

  def StartTests.process_logs(rspec_output,saved_stdall)
    rspec_output.flush
    saved_stdall.flush

    #create build log dir
    FileUtils.mkdir_p(LOGS_PATH+"/#{@options[:build_id]}")

    #flush out IO stings into log files
    rspeclog=File.new(LOGS_PATH+"/#{@options[:build_id]}/rspec.log","w+")
    rspeclog.write("Build: #{@options[:build_id]}\n\n rspec log:\n\n #{rspec_output.string}\n\n")
    rspeclog.close

    debuglog=File.new(LOGS_PATH+"/#{@options[:build_id]}/rspec_debug.log","w+")
    debuglog.write("Build: #{@options[:build_id]} debug log: \n\n #{saved_stdall.string}")
    debuglog.close
    
  end

  def StartTests.process_tests(save_output)

    #create build log before running tests for chef logs
    FileUtils.mkdir_p(LOGS_PATH+"/#{@options[:build_id]}")

    rc=0
    
    @auto_tests.each do |spec_file|
      #reload spec file for each run??
      #(see http://www.rubydoc.info/github/rspec/rspec-core/RSpec/Core/Runner#run-class_method)
      #load SPECHELPER_FILE #comment for now since it loads twice

      print("\n\n-----------------------Gonna run #{spec_file}---------------------------------\n\n")

      #setup spec args array
      spec_args=[spec_file]+RSPEC_ARGS
      rc=RSpec::Core::Runner.run(spec_args,save_output,save_output)

    end
    return rc
  end

  def StartTests.process_notify(send_output)
    failmsg=<<FAILURE_MSG
From: <tester@seed>
To: #{@email_notify_addrs.join(',')}
Subject: auto tests failure build:#{@options[:build_id]}

Automated tests: #{@auto_tests} for build:#{@options[:build_id]} failed.

RSpec test results:

#{send_output.string}

FAILURE_MSG

    smtp=Net::SMTP.new(@email_smtp_server, @email_smtp_port) 
#uncomment to debug mailing issues
#    smtp.set_debug_output(STDOUT)
    smtp.enable_starttls
    smtp.start(@email_smtp_server,'autotester','Crabba1t',:login)do |smtp|
      # Use the SMTP object smtp only in this block.
      smtp.send_message(failmsg,'tester@seed',@email_notify_addrs)
    end
  end


  def StartTests.run


    #create IO stream to capture output for logging
    #in spec_helper.rb created a new $stdout stream for 'logger'
    test_output=StringIO.new    
    
    rc=0
    rc=process_tests(test_output)
    
    process_logs(test_output,$stdout)
    
    if rc != 0 
      process_notify(test_output)
    end
    return rc
  end

end #end  module StartTests

include StartTests
#check if we are running standalone
if __FILE__ == $0
    #process args
    StartTests.process_args

    if StartTests::options[:foreground]
      StartTests.run
    else
      #fork child
      begin
         pid = fork
      rescue StandardError => e
        print "\n\nError: exception occured during fork\n"
        print "\n#{e.backtrace}"
    
        print "\nCannot fork...exiting\n"
        exit(1)
      end

      if pid.nil?
        #I am child
        Process.setsid
        print "\nIn child...running tests\n"
        #clone parents out and err
        #then reopen
        oldout=STDOUT.clone
        olderr=STDERR.clone


        STDIN.reopen '/dev/null'
        STDOUT.reopen oldout
        STDERR.reopen olderr

        exit(StartTests.run)

      else
        #I am parent...ignore child exit sig
        print "\nI am parent...exiting\n"
        trap("CHLD","IGNORE")
        #close parent streams

        STDIN.close
        STDOUT.close
        STDERR.close

        exit(0)
      end
    end

end
