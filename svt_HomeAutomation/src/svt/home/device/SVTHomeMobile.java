package svt.home.device;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Random;
import java.util.Stack;
import java.util.concurrent.LinkedBlockingQueue;

import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import svt.framework.device.SVT_Device;
import svt.framework.messages.TopicMessage;
import svt.home.messages.SVTHome_CommandMessage;

public class SVTHomeMobile extends SVT_Device {
	HashMap<String, CapacityQueue> deviceData = new HashMap<>();
	Stack<TopicMessage> stack = new Stack<TopicMessage>();
	ArrayList<JSONObject> deviceIds = new ArrayList<JSONObject>();
	private java.util.Random randGen = new Random(300000);
	private long lastCommandTime;
	private long commandInterval;

	// * – keep last 100 points for each device
	// * #create a hashmap that will hold lists?
	// * #consider wrapping a queue class that removes the last element before
	// insert
	// * -Send status command to each device
	// * # double check that sensors and controls have status command
	// * # consider putting it in Device
	// * -Send enable command to any disabled devices
	// * # likewise with status command
	// * -Start sending a semi-random series of commands to each control
	// * #
	// * -Run until stop
	// * #
	// * -Disable a random set of devices
	// * #
	// * -log out of main office

	public SVTHomeMobile(long custId, long deviceId, String deviceType) {
		super(custId, deviceId, deviceType);
	}

	@SuppressWarnings("unchecked")
	public int initApp() {
		// * -Startup
		// * -Log into main office #what does it mean to log in?
		// *

		// * -If not registered to main office, register mobile device using
		// customerID, device ID and device type
		// * #send a registration. Main office will ignore if already registered

		SVTHome_CommandMessage msg;
		registerDevice();
		// * -Subscribe to data feeds for each device
		msg = new SVTHome_CommandMessage("list", custId, 0L, "");
		transportThread.sendEvent(msg);
		TopicMessage replyMsg = stack.pop();
		String replyText = "";
		try {
			replyText = replyMsg.getMessageText();
		} catch (MqttException e) {
			e.printStackTrace();
		}
		if (replyText != "") {
			JSONObject replyObj = (JSONObject) JSONValue.parse(replyText);
			deviceIds.addAll(replyObj.entrySet());
		}
		return 0;
	}
	public void sendSensorCommand() {
		SVTHome_CommandMessage commandEvent = null;

		switch (randGen.nextInt(6)) {
		//"set"
		case 0:
			commandEvent = buildCommand("set");
			commandEvent.addData("value",String.valueOf(randGen.nextInt(100)));
			break;
		//"clear"
		case 1:
			commandEvent = buildCommand("clear");
			break;
		//"reset"
		case 2:
			commandEvent = buildCommand("reset");
			break;
		//"down"
		case 3:
			commandEvent = buildCommand("down");
			commandEvent.addData("value",String.valueOf(randGen.nextInt(5)));
			break;
		//"up"
		case 4:
			commandEvent = buildCommand("up");
			commandEvent.addData("value",String.valueOf(randGen.nextInt(5)));
			break;
		//"enable"
		case 5:
			commandEvent = buildCommand("enable");
			break;
		//"disable"
		case 6:
			commandEvent = buildCommand("disable");
			break;
		}
		transportThread.sendEvent(commandEvent);
	}
	public void sendControlCommand() {
		SVTHome_CommandMessage commandEvent = null;

		switch (randGen.nextInt(6)) {
		//"set"
		case 0:
			commandEvent = buildCommand("set");
			switch (randGen.nextInt(3)) {
			case 0:
				commandEvent.addData("value","off");
				break;
			case 1:
				commandEvent.addData("value","low");
				break;
			case 2:
				commandEvent.addData("value","medium");
				break;
			case 3:
				commandEvent.addData("value","high");
				break;
				
			}
			break;
		//"clear"
		case 1:
			commandEvent = buildCommand("clear");
			break;
		//"reset"
		case 2:
			commandEvent = buildCommand("reset");
			break;
		//"down"
		case 3:
			commandEvent = buildCommand("down");
			break;
		//"up"
		case 4:
			commandEvent = buildCommand("up");
			break;
		//"enable"
		case 5:
			commandEvent = buildCommand("enable");
			break;
		//"disable"
		case 6:
			commandEvent = buildCommand("disable");
			break;
		}
		transportThread.sendEvent(commandEvent);
	}
	@SuppressWarnings("unchecked")
	public void sendCommand() {
		// sends commands in random intervals less than 5 minutes.
		long currentTime = System.currentTimeMillis();
		if ((currentTime - lastCommandTime) > commandInterval) {
			commandInterval = randGen.nextLong();
			lastCommandTime = currentTime;
			SVTHome_CommandMessage msg;
			msg = new SVTHome_CommandMessage("list", custId, 0L, "");
			transportThread.sendEvent(msg);
			TopicMessage replyMsg = stack.pop();
			String replyText = "";
			try {
				replyText = replyMsg.getMessageText();
			} catch (MqttException e) {
				e.printStackTrace();
			}
			JSONObject replyObj = (JSONObject) JSONValue.parse(replyText);
			deviceIds.addAll(replyObj.entrySet());
			for (Iterator iterator = deviceIds.iterator(); iterator.hasNext();) {
				Integer currIndex = (Integer) iterator.next();
				JSONObject jsonObj = deviceIds.get(currIndex.intValue());
				for (Iterator<String> iterator2 = jsonObj.keySet().iterator(); iterator2
						.hasNext();) {
					String currDeviceID = (String) iterator2.next();
					String currDeviceType = ((String) jsonObj.get(currDeviceID))
							.toLowerCase();
					switch (currDeviceType) {
					case "sensor":
						if (randGen.nextBoolean()) {
						sendSensorCommand();
						}
						break;
					case "control":
						if (randGen.nextBoolean()) {
						sendControlCommand();
						}
						break;
					case "mobile":
						break;
					default:
						break;
					}
				}

			}

		}
	}

	public SVTHome_CommandMessage buildCommand(String msg) {
		SVTHome_CommandMessage commandMsg = new SVTHome_CommandMessage(
				"command", custId, deviceId, deviceType);
		commandMsg.message = msg;
		// commandEvent.requestTopic = replyTopic;
		commandMsg.replyTopic = null;
		return commandMsg;
	}
	public SVTHome_CommandMessage buildCommand(String msg, String value) {
		SVTHome_CommandMessage commandMsg = new SVTHome_CommandMessage(
				"command", custId, deviceId, deviceType);
		commandMsg.message = msg;
		// commandEvent.requestTopic = replyTopic;
		commandMsg.replyTopic = null;
		return commandMsg;
	}

	@Override
	public boolean processCommand(String topic, String command) {
		JSONObject replyObj = (JSONObject) JSONValue.parse(command);
		String cmd = (String) replyObj.get("Command");
		if (cmd != null && command == "reply") {
			MqttMessage mqttMsg = new MqttMessage();
			mqttMsg.setPayload(command.getBytes());
			TopicMessage msg = new TopicMessage(topic,mqttMsg);
			stack.push(msg);
		}
		return true;
	}

	@Override
	public Object getCurrentValue() {
		// TODO Auto-generated method stub
		sendCommand();
		return null;
	}

	public void addSubscriptions() {
//		for (Iterator<Long> iterator = deviceIds.iterator(); iterator.hasNext();) {
//			Long id = (Long) iterator.next();
//
//		}
		return;
	}

	/******************************************************************
	 * 
	 * Nested classes follow
	 * 
	 ******************************************************************/
	public class CapacityQueue extends LinkedBlockingQueue<String> {

		/**
	 * 
	 */
		private static final long serialVersionUID = 1L;

		public CapacityQueue() {
			super();
		}

		public CapacityQueue(Collection<? extends String> c) {
			super(c);
		}

		public CapacityQueue(int capacity) {
			super(capacity);
		}

		@Override
		public void put(String e) throws InterruptedException {
			makeRoom();
			super.put(e);
		}

		@Override
		public boolean add(String e) {
			makeRoom();
			return super.add(e);
		}

		private void makeRoom() {
			if (remainingCapacity() == 0) {
				remove();
			}
		}

	}

}
