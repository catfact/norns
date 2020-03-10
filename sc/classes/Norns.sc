// the Norns interface, a singleton class
// receives OSC from *matron* and manages the current NornsEngine
Norns {
	//--- state variables
	//--- these have getters for introspection / debug purposes
	// the audio server
	classvar <server;
	// current NornsEngine subclass instance
	classvar <engine;
	// available OSC functions
	classvar <oscfunc;
	// address of remote client
	classvar <remoteAddr;
	// port for sending OSC to matron
	classvar <txPort = 8888;
	// server port
	classvar <serverPort = 57122;
	// boot completion flag
	classvar complete = 0;
	
	//--- configuration flags
	// if true, use a remote server (still on localhost)
	// otherwise, use default (internal) server
	classvar useRemoteServer = false;

	*initClass {
		StartUp.add { // defer until after sclang init


			postln("\n-------------------------------------------------");
			postln(" Norns startup");
			postln("");
			postln(" OSC rx port: " ++ NetAddr.langPort);
			postln(" OSC tx port: " ++ txPort);
			postln(" server port: " ++ serverPort);
			postln("--------------------------------------------------\n");

			remoteAddr = NetAddr("127.0.0.1", txPort);

			"SC_JACK_DEFAULT_INPUTS".setenv("");
			"SC_JACK_DEFAULT_OUTPUTS".setenv("");

			Norns.startBoot;
		}
	}

	*runShellCommand { arg str;
		var p,l;
		p = Pipe.new(str, "r");
		l = p.getLine;
		while({l.notNil}, {l.postln; l = p.getLine; });
		p.close;
	}

	*startBoot {
		if(useRemoteServer, {
			Server.default = Server.remote(\norns, NetAddr("127.0.0.1", serverPort));
			server = Server.default;
			server.doWhenBooted {
				Norns.finishBoot;
			};
		}, {
			Server.scsynth;
			server = Server.local;
			// doesn't work on supernova - "invallid argument" - too big?
			// server.options.memSize = 2**16;
			server.latency = 0.05;
			server.waitForBoot {
				Norns.finishBoot;
			};
		});
	}

	*finishBoot {
		Norns.runShellCommand("jack_connect \"crone:output_5\" \"SuperCollider:in_1\"");
		Norns.runShellCommand("jack_connect \"crone:output_6\" \"SuperCollider:in_2\"");

		Norns.runShellCommand("jack_connect \"SuperCollider:out_1\" \"system:playback_1\"");
		Norns.runShellCommand("jack_connect \"SuperCollider:out_2\" \"system:playback_2\"");

		// for off-norns development, probably want to connect to system I/O ports
		/*
		Norns.runShellCommand("jack_connect \"system:capture_1\" \"SuperCollider:in_1\"");
		Norns.runShellCommand("jack_connect \"system:capture_2\" \"SuperCollider:in_2\"");
		Norns.runShellCommand("jack_connect \"SuperCollider:out_1\" \"crone:input_5\"");
		Norns.runShellCommand("jack_connect \"SuperCollider:out_2\" \"crone:input_6\"");
		*/
		
		Norns.initOscRx;

		complete = 1;

		/// test..
		// { SinOsc.ar([218,223]) * 0.125 * EnvGen.ar(Env.linen(2, 4, 6), doneAction:2) }.play(server);

	}

	*loadEngineClass { arg name;
		var class;
		class = NornsEngine.allSubclasses.detect({ arg n; n.asString == name.asString });
		if(class.notNil, {
			fork {
				if(engine.notNil, {
					var cond = Condition.new(false);
					postln("free engine: " ++ engine);
					engine.deinit({ cond.test = true; cond.signal; });
					cond.wait;

				});
				class.new({
					arg theEngine;
					postln("-----------------------");
					postln("-- norns: done loading engine, starting reports");
					postln("--------");

					this.engine = theEngine;
					postln("engine: " ++ this.engine);

					this.reportCommands;
					this.reportPolls;
				});
			}
		}, {
		   postln("warning: didn't find engine: " ++ name.asString);
		   this.reportCommands;
		   this.reportPolls;
		});
	}

	*loadEngineScript { arg path;
		// TODO..
	}

	// start a thread to continuously send a named report with a given interval
	*startPoll { arg idx;
		var poll = NornsPollRegistry.getPollFromIndex(idx);
		if(poll.notNil, {
			poll.start;
		}, {
			postln("startPoll failed; couldn't find index " ++ idx);
		});
	}

	*stopPoll { arg idx;
		var poll = NornsPollRegistry.getPollFromIndex(idx);
		if(poll.notNil, {
			poll.stop;
		}, {
			postln("stopPoll failed; couldn't find index " ++ idx);
		});
	}

	*setPollTime { arg idx, dt;
		var pt = NornsPollRegistry.getPollFromIndex(idx);
		if(pt.notNil, {
			pt.setTime(dt);
		}, {
			postln("setPollTime failed; couldn't find index " ++ idx);
		});
	}

	*requestPollValue { arg idx;
		var poll = NornsPollRegistry.getPollFromIndex(idx);
		if(poll.notNil, {
			poll.requestValue;
		});
	}

	*reportEngines {
		var names = NornsEngine.allSubclasses.select {
			|class| class.name.asString.beginsWith("Engine_");
		}.collect({ arg n;
			n.asString.split($_).drop(1).join($_)
		});
		postln('engines: ' ++ names);
		remoteAddr.sendMsg('/report/engines/start', names.size);
		names.do({ arg name, i;
			remoteAddr.sendMsg('/report/engines/entry', i, name);

		});
		remoteAddr.sendMsg('/report/engines/end');
	}

	*reportCommands {
		var commands = engine !? _.commands;
		remoteAddr.sendMsg('/report/commands/start', commands.size);
		commands.do({ arg cmd, i;
			postln('command entry: ' ++ [i, cmd.name, cmd.format]);
			remoteAddr.sendMsg('/report/commands/entry', i, cmd.name, cmd.format);
		});
		remoteAddr.sendMsg('/report/commands/end');
	}

	*reportPolls {
		var num = NornsPollRegistry.getNumPolls;
		remoteAddr.sendMsg('/report/polls/start', num);
		num.do({ arg i;
			var poll = NornsPollRegistry.getPollFromIndex(i);
			postln(poll.name);
			// FIXME: polls should just have format system like commands?
			remoteAddr.sendMsg('/report/polls/entry', i, poll.name, if(poll.type == \value, {0}, {1}));
		});

		remoteAddr.sendMsg('/report/polls/end');
	}

	*initOscRx {
		oscfunc = (

			// @module crone
			// @alias crone

			/// send a `/crone/ready` response if Norns is done starting up,
			/// otherwise send nothing
			'/ready':OSCFunc.new({
				arg msg, time, addr, recvPort;
				if(complete==1) {
					postln(">>> /crone/ready");
					remoteAddr.sendMsg('/crone/ready');
				}
			}, '/ready'),

			// @section report management

			/// begin OSC engine report sequence
			'/report/engines':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.reportEngines;
			}, '/report/engines'),

			/// begin OSC command report sequence
			'/report/commands':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.reportCommands;
			}, '/report/commands'),

			/// begin OSC poll report sequence
			'/report/polls':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.reportPolls;
			}, '/report/polls'),

			// free the current engine
			'/engine/free':OSCFunc.new({
				if(engine.notNil, { engine.deinit; });
			}, '/engine/free'),

			// load an engine by class name
			// @param engine name (string)
			'/engine/load/name':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.loadEngineClass('Engine_' ++ msg[1]);
			}, '/engine/load/name'),
			
			// load an engine by running a script
			// @param script file  (full path)
			'/engine/load/script':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.loadEngineScript(msg[1]);
			}, '/engine/load/script'),


			// start a poll
			// @param poll index (integer)
			'/poll/start':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.startPoll(msg[1]);
			}, '/poll/start'),

			// @param poll index (integer)
			'/poll/stop':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.stopPoll(msg[1]);
			}, '/poll/stop'),

			/// set the period of a poll
			// @param poll index (integer)
			// @param poll period(float)
			'/poll/time':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.setPollTime(msg[1], msg[2]);
			}, '/poll/time'),


			/// set the period of a poll
			// @param poll index (integer)
			'/poll/request/value':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.requestPollValue(msg[1]);
			}, '/poll/request/value'),

			// recompile the sclang library!
			'/recompile':OSCFunc.new({
				postln("recompile...");
				thisProcess.recompile;
			}, '/recompile'),
		);

	}
}

