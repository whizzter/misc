import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Random;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.lang.ref.WeakReference;

/**
 * Super primitive threaded tcp forwarding program with support for delaying messages.
 * 
 * @author Jonas Lund (whizzter@gmail.com)
 *
 */
public class tcpfwd {
	static class QueuedTimer {
		class Evt {
			long time;
			Runnable r;
			Evt(long ti,Runnable ru) { this.time=ti; this.r=ru; }
		}
		ConcurrentLinkedQueue<Evt> evts=new ConcurrentLinkedQueue<Evt>();

		// we have a static start method since we only want a weak-ref to the queue so
		// that GC'ing the QueuedTimer will cause the thread to stop aswell.
		static private void start(WeakReference<ConcurrentLinkedQueue<Evt>> ref) {
			new Thread(()->{
				while(true) {
					// start of loop, get a hard ptr to the event queue
					ConcurrentLinkedQueue<Evt> evts=ref.get();
					if (evts==null)
						return; // stop thread if evts doesn't exist no more.
					// always wake up atleast once every 100ms
					Evt e=evts.peek();
					long cts=System.currentTimeMillis();
					while(e!=null) {
						if (e.time>cts)
							break; // event in future, do not process!
						e=evts.poll(); // remove first to process
						e.r.run();
						e=evts.peek();
						cts=System.currentTimeMillis(); // take new timesample since the task might've taken long
					}
					long nextWake=cts+30;
					if (e!=null && e.time<nextWake)
						nextWake=e.time;
					try {
						Thread.sleep(nextWake-cts);
					} catch (InterruptedException iex) {}
				}
			}).start();
		}

		QueuedTimer() {
			start( new WeakReference<ConcurrentLinkedQueue<Evt>>(evts) );
		}

		void schedule(long delay,Runnable r) {
			evts.add(new Evt(System.currentTimeMillis()+delay,r));
		}
	}

	public static void main(String[] args) {
		// setup a connection string array
		ArrayList<String> basedata=new ArrayList<String>();
		// by default don't delay messages.
		int[] delays=new int[2];
		int[] jitters=new int[2];
		// parse arguments, putting non-args into basedata and everything starting with a - is treated as an argument.
		for (String arg:args) {
			// take everything starting with a -
			if (arg.startsWith("-")) {
				// just a simple if-else chain here
				if (arg.startsWith("-delay:")) {
					delays[0]=Integer.parseInt(arg.substring(7));
					if (delays[1]==0)
						delays[1]=delays[0];
				} else if (arg.startsWith("-jitter:")) {
					jitters[0]=jitters[1]=Integer.parseInt(arg.substring(8));
				} else if (arg.startsWith("-delay2:")) {
					delays[1]=Integer.parseInt(arg.substring(8));
				} else {
					// error out on unknown args.
					System.err.println("Unknown arg "+arg);
					System.exit(-1);
				}
			} else {
				basedata.add(arg);
			}
		}
		// we require 3 parameters to exist, srcport, remotehost and remoteport.
		if (basedata.size()<3) {
			System.err.println("Usage: [options] srcport remotehost remoteport");
			System.exit(-1);
		}
		
		// config is sufficient.
		try {
			// start listening.
			@SuppressWarnings("resource")
			ServerSocket ss=new ServerSocket(Integer.parseInt(basedata.get(0)));

			//// setup a daemonized timer if needed.
			//Timer timer=new Timer(true);

			// accept sockets forever.
			while(true) {
				Socket[] sockets=new Socket[2];
				sockets[0] = ss.accept();
				System.err.println("Accepted client from "+sockets[0]);
				
				// now open a connection to the remote side.
				sockets[1] = new Socket(basedata.get(1), Integer.parseInt(basedata.get(2)));
				System.err.println("Connected to our forwarding location at "+sockets[1]);

				// now tie together output and input sides of each socket
				for (int i=0;i<2;i++) {
					QueuedTimer qt=new QueuedTimer();
					// now connect both sides.
					// first get an inputstream from one of the sides
					final InputStream is=sockets[i].getInputStream();
					// then take the other
					final Socket other=sockets[i^1];
					// and open an output stream to it.
					final OutputStream os=other.getOutputStream();
					
					// get our delay
					int delay=delays[i];
					int jitter=jitters[i];
					
					// create and start a new thread for this direction.
					new Thread(()->{
						Random rnd=new Random();
						// The thread started here will block reading in one direction and then send over the data (possibly after an delay)
						byte[] dataBuf=new byte[4096];
						try {
							// do this until we've failed.
							while(true) {
								// try reading a full dataBuf (if we really read a full buffer or not does not matter)
								int rc = is.read(dataBuf);
								// if the socket is closed we need to break the loop
								if (rc<0)
									break;
								// should not happen but we'll just continue in this case.
								if (rc==0)
									continue;
								// if no delay is given we send out the read data directly
								if (delay==0)
									os.write(dataBuf, 0, rc); // rc is positive here.
								else {
									int rdel=jitter>0?rnd.nextInt(jitter):0;
									// if delayed we need to make a copy of the data to be sent (just copy the amount read)
									byte[] out = Arrays.copyOf(dataBuf, rc);
									// and schedule a timer to do the actual sending.
									qt.schedule(delay+rdel,()->{  //new TimerTask() {public void run() {
											// this function will be run after a delay to send the data in out
											try {
												os.write(out,0,out.length); // send everything
											} catch (IOException ioex) {}
									}); //}}, delay);
								}
							}
						} catch (IOException e) {} // the catch here doesn't really do anything but is here to ensure that we try closing the other side.
						try {
							// close the other socket when this side fails so that we don't have lingering threads.
							other.close();
						} catch (IOException e) {
							e.printStackTrace();
						}
					}).start();
				}
			}
		} catch (NumberFormatException | IOException e) {
			// just dump out data if we fail ( we shouldn't unless there's a connection error or a misspecified parameter)
			e.printStackTrace();
			System.err.println("Possible errors, misspecified listening or connection port, or a remote host not accepting connections");
			System.exit(-1);
		}
	}
}
