import java.io.*;
import java.util.*;
import javax.microedition.io.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.wireless.messaging.*;

/**
 * An example MIDlet with simple "Hello" text and an Exit command.
 * Refer to the startApp, pauseApp, and destroyApp
 * methods so see how each handles the requested transition.
 *
 * @author  shai
 * @version
 */
public class NCISlave extends MIDlet implements CommandListener, Runnable {
    /**
     * Messages pending for the server
     */
    private Vector messageQueue = new Vector();
    private static final int SLEEP_LENGTH = 50;
    private Command exitCommand; // The exit command
    private Command connectCommand; // The connect command
    private Command logCommand; // The log command
    private Command statusCommand; // The status command
    private Command confCommand; // The configuration command
    private Display display;    // The display for this MIDlet
    private TextBox url;
    private Form    configuration;
    private TextBox getIp;
    private Form status;
    private StringItem  statusConect;
    private TextField  statusSend;
    private TextField  statusRecieve;
    private StringBuffer logBuffer;
    private int maxLenLogBuffer = 1000;
    private volatile String address = null;
    private Vector listeners = new Vector();
    private static Hashtable urlMap = new Hashtable();
    private int NumOfSMSSend;
    private int NumOfSMSRecv;
    private int NumOfActions;
    private String LastAction;
    private String Connection;
    private String Connection1;
    private String TCKPort;
    private String TCKIPAddress;
    MessageConnection smsconn, mmsconn;

    /**
     * Start up the Hello MIDlet by creating the TextBox and associating
     * the exit command and listener.
     */
    public void startApp() {
        try {
            System.out.println("Starting MIDlet");
            display = Display.getDisplay(this);
            exitCommand = new Command("Exit", Command.EXIT, 2);
            connectCommand = new Command("Connect", Command.SCREEN, 1);
	    logCommand = new Command("Log", Command.SCREEN, 1);
	    statusCommand = new Command("Status", Command.SCREEN, 1);
	    confCommand = new Command("Configuration", Command.SCREEN, 1);
            TCKPort = getAppProperty("TCKPort");
            TCKIPAddress = getAppProperty("TCKIPAddress");
            getIp = new TextBox("Enter TCK server address", TCKIPAddress, 256, 0);
	    address = getIp.getString();
	    url = new TextBox("Logging", null,10000,0);
	    configuration = new Form("Configuration");
	    configuration.append("Host: " + address);
	    configuration.append("\nPort: " + TCKPort);
	    configuration.append("\nPhone number: 5550000");
	    status = new Form("Status");
	    statusConect = new StringItem("Connection status", null);
	    statusSend = new TextField("Total Send Messages", null, 3, TextField.UNEDITABLE);
	    statusRecieve = new TextField("Total Recieved Messages", null, 3, TextField.UNEDITABLE);
	    status.append(statusConect);
	    status.append(statusSend);
	    status.append(statusRecieve);
	    logBuffer = new StringBuffer(maxLenLogBuffer);   

            configuration.addCommand(exitCommand);
            configuration.addCommand(connectCommand);
	    configuration.setCommandListener(this);

	    getIp.addCommand(exitCommand);
	    getIp.addCommand(confCommand);
	    getIp.setCommandListener(this);

            
            url.setCommandListener(this);
	    url.addCommand(exitCommand);
	    url.addCommand(statusCommand);    

	    status.addCommand(exitCommand);
	    status.addCommand(logCommand);
	    status.setCommandListener(this);
            
            display.setCurrent(getIp);
            
            Connection = getAppProperty("listen0");
            smsconn = (MessageConnection)Connector.open(Connection);
            smsconn.setMessageListener(new MessageListenerAdapter(Connection));
            Connection1 = getAppProperty("listen1");
            mmsconn = (MessageConnection)Connector.open(Connection1);
            mmsconn.setMessageListener(new MessageListenerAdapter(Connection1));
            
        } catch(Exception err) {
            reportError("Error", err);
        }
    }
    
    private void reportError(String label, Exception err) {
        err.printStackTrace();
        url.setString(label + "  " + err.getClass().getName() + ": " + err.getMessage());
        Alert error = new Alert(err.getClass().getName(), err.getMessage(), null, AlertType.ERROR);
        display.setCurrent(error);
    }
    
    public void pauseApp() {
    }
    
    /**
     * Destroy must cleanup everything not handled by the garbage collector.
     * In this case there is nothing to cleanup.
     */
    public void destroyApp(boolean unconditional) {
        address = null;
    }
    
    /*
     * Respond to commands, including exit
     * On the exit command, cleanup and notify that the MIDlet has been destroyed.
     */
    public void commandAction(Command c, Displayable s) {
        if (c == exitCommand) {
            destroyApp(false);
            notifyDestroyed();
            return;
        }
        
	 if (c == confCommand) {
            display.setCurrent(configuration);
            return;
        }
        
        if (c == connectCommand) {
            if(address != null) {
                address = null;
            } else {
                address = TCKIPAddress;
		display.setCurrent(status);
		statusConect.setText("Trying to Connect to TCK Server on host: " + address +"\nTCK port: " +TCKPort);
	
                new Thread(this).start();                
            }
            return;
        }
	 if (c == logCommand) {
	    display.setCurrent(url);
	    return;
	}
	  if (c == statusCommand) {
	    display.setCurrent(status);
	    return;
	}
    }
    
    /**
     * This is the thread that polls the server for commands
     */
    public void run() {
	String tmpString = null;

        long time = System.currentTimeMillis();
        while(address != null) {
            //System.out.println("Polling server in: " + (System.currentTimeMillis() - time));
            time = System.currentTimeMillis();
            try {
                Message current = null;
                synchronized(this) {
                    if(messageQueue.size() > 0) {
                        current = (Message)messageQueue.elementAt(0);
                        messageQueue.removeElementAt(0);
                    }
                }
                // used to avoid lengthly synchronization holding the
                // event thread
                if(current != null) {
                    System.out.println("message not null!");
                    sendMessageToServer(current);
                }

                DataInputStream input = Connector.openDataInputStream("http://" + address + ":"+TCKPort+"/read");

		statusConect.setText("\nCONNECTED TO TCK SERVER.\n\nWaiting for SMS on: " + Connection);
		tmpString = tmpString.valueOf(NumOfSMSSend);
		statusSend.setString(tmpString);
		tmpString = tmpString.valueOf(NumOfSMSRecv);
		statusRecieve.setString(tmpString);

		
                // do we have anything pending?
                if(!input.readBoolean()) {
                    int listenerCount = input.readInt();
                    for(int iter = 0 ; iter <  listenerCount ; iter++) {
                        String currentURL = input.readUTF();
                        if(!listeners.contains(currentURL)) {
                            listeners.addElement(currentURL);
                            url.setString("Try open :" + currentURL);
                            ((MessageConnection)Connector.open(currentURL)).setMessageListener(new MessageListenerAdapter(currentURL));
                            url.setString("After open :" + currentURL);
                        }
                    }
                    synchronized(this) {
                        wait(SLEEP_LENGTH);
                    }
                    input.close();
                    continue;
                }
                
                // send an SMS message to the appropriate address
                String address = input.readUTF();

                // is the message a text/binary/multipart message
                int type = input.readInt();
                if(type == 0) {
                    String text = input.readUTF();
                    sendTextMessage(address, text);
                } else if (type == 1) {
                    byte[] data = new byte[input.readInt()];
                    input.readFully(data);
                    sendBinaryMessage(address, data);
                } else if (type == 2) {
                    url.setString("Unknown message type to send:" + type);
                }
                input.close();
            } catch(Exception err) {
                url.setString("Retrying on connection error " + err.getClass() + ": " + err.getMessage());    
            }

	    	setBuffer();
	 
        }
    }
    
    private String urlEncode(byte[] data) throws IOException {
        StringBuffer out = new StringBuffer();
        ByteArrayInputStream bIn = new ByteArrayInputStream(data);
        int c = bIn.read();
        while (c >= 0) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '*' || c == '_')
                out.append((char) c);
            else if (c == ' ')
                out.append('+');
            else {
                if (c < 128) {
                    appendHex(c,out);
                } else if (c < 224) {
                    appendHex(c,out);
                    appendHex(bIn.read(),out);
                } else if (c < 240) {
                    appendHex(c,out);
                    appendHex(bIn.read(),out);
                    appendHex(bIn.read(),out);
                }                
            }
            c = bIn.read();
        }
        return out.toString();
    }
    
    private void appendHex(int arg0, StringBuffer buff){
        buff.append('%');
        if (arg0<16) buff.append('0');
        buff.append(Integer.toHexString(arg0));
    }
    
    /**
     * Sends a message received by this phone to the server
     */
    private void sendMessageToServer(final Message message) {
        new Thread() {
            public void run() {
                while(true) {
                    try {
                        StringBuffer url1 = new StringBuffer("http://");
                        url1.append(address);
                        url1.append(":"+TCKPort+"/write;");
                        if(message instanceof TextMessage) {
                            LastAction = "Received TEXT SMS from:"+urlMap.get(message);
                            NumOfSMSRecv++;

                            url1.append("t;");
                            url1.append(urlEncode(((TextMessage)message).getPayloadText().getBytes("UTF-8")));
                            url1.append(";");
                        } else if (message instanceof BinaryMessage) {
                            LastAction = "Received BINARY SMS from:"+urlMap.get(message);
                            
                            NumOfSMSRecv++;
                            url1.append("b;");
                            url1.append(urlEncode(((BinaryMessage)message).getPayloadData()));
                            url1.append(";");
                        } else {
                            throw new java.io.IOException("Unexpected message kind");
                        }
                        System.out.println("Sending message to server address: " +urlMap.get(message));
                        url1.append(urlEncode(((String)urlMap.get(message)).getBytes("iso8859-1")));
                        url1.append(";");
                        long time = 0;
                        if (message.getTimestamp() != null) { 
                            time = message.getTimestamp().getTime();
                        }
                        url1.append(time);
                        DataInputStream input = Connector.openDataInputStream(url1.toString());
                        input.readBoolean();
                    } catch(Exception err) {
                        reportError("Send to server error", err);
                        continue;
                    }

		   setBuffer();
	
                    // will only return if no exception was thrown
                    return;
                }
            }
        }.start();
    }

    private void sendBinaryMessage(String address, byte[] data) throws IOException {
        try {
            BinaryMessage message = (BinaryMessage)smsconn.newMessage(
                MessageConnection.BINARY_MESSAGE);
            System.out.println("sendBinaryMessage to:"+address);
            message.setAddress(address);
            message.setPayloadData(data);
            smsconn.send(message);
            LastAction = "Sending BINARY SMS to:" + address;
            NumOfSMSSend++;
        } catch(Exception e) {
            reportError("sendBinaryMessage error", e);
        }
    }

    private void sendTextMessage(String address, String text) throws IOException {
        try {
            TextMessage txtmessage = (TextMessage)smsconn.newMessage(
            MessageConnection.TEXT_MESSAGE);
            txtmessage.setAddress(address);
            txtmessage.setPayloadText(text);
            smsconn.send(txtmessage);
            LastAction = "Sending TEXT SMS to:" + address;
            NumOfSMSSend++;
        } catch(Exception e) {
            reportError("sendBinaryMessage error", e);
        }
    }

    private void setBuffer(){
	String tmpString;
	int currentBufLen = logBuffer.length();
	int testSize = maxLenLogBuffer-LastAction.length();
	int newLength = currentBufLen+maxLenLogBuffer;

	if(currentBufLen > testSize) {
	    logBuffer.setLength(newLength);
	    logBuffer.delete(currentBufLen, newLength);
	}

	NumOfActions++;
	logBuffer.append("\n"+NumOfActions+") "+LastAction);
	tmpString = logBuffer.toString();
	url.setString(tmpString);
    }
    

    class MessageListenerAdapter implements MessageListener, Runnable {
        private String url;
        private MessageConnection conn;
     
        public MessageListenerAdapter(String url) {
            this.url = url;
        }
        
        public void notifyIncomingMessage(MessageConnection messageConnection) {
        conn = messageConnection;
            new Thread(this).start();     
        }

        public void run() {
            System.out.println("message income>>");

            try {
                Message m = conn.receive();

                System.out.println("received from:"+m.getAddress());
                if (m instanceof MultipartMessage) {
                    String[] to = ((MultipartMessage)m).getAddresses("to");
                    if (to != null) {
                        for (int i = 0; i < to.length; i++) {
                            System.out.println("it's to:"+to[i]);
                        }
                    }
                }

                urlMap.put(m, url);
                synchronized(this) {
                    messageQueue.addElement(m);
                }
                System.out.println("message income<<");
            } catch(Exception err) {
                reportError("Message queue error", err);
            }
        }
    }

}

