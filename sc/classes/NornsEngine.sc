// an audio "engine."
// maintains some DSP processing and provides control over parameters and analysis results
// new engines should inherit from this
CroneEngine {

	// list of registered commands
	var <commands;
	var <commandNames;

	// list of registered polls
	var <pollNames;

	*new { arg completeFunc;
		^super.new.init(completeFunc);
	}

	init { arg completeFunc;
		commands = List.new;
		commandNames = Dictionary.new;
		pollNames = Set.new;
		fork {
			this.alloc;
			completeFunc.value(this);
		};
	}

	alloc {
		// subclass responsibility to allocate server resources.
		// his method is called in a Routine, so Server.sync can be used
	}

	addPoll { arg name, func, periodic=true;
		name = name.asSymbol;
		CronePollRegistry.register(name, func, periodic:periodic);
		pollNames.add(name);
		^CronePollRegistry.getPollFromName(name);
	}

	// deinit is called in a routine
	deinit { arg completeFunc; 
		postln("CroneEngine.free");
		commands.do({ arg com;
			com.oscdef.free;
		});
		pollNames.do({ arg name;
			CronePollRegistry.remove(name);
		});
		// subclass responsibility to implement free
		this.free;
		Crone.server.sync;
		completeFunc.value(this);
	}


	addCommand { arg name, format, func;
		var idx, cmd;
		name = name.asSymbol;
		postln([ "CroneEngine adding command", name, format, func ]);
		if(commandNames[name].isNil, {
			idx = commandNames.size;
			commandNames[name] = idx;
			cmd = Event.new;
			cmd.name = name;
			cmd.format = format;
			cmd.oscdef = OSCdef(name.asSymbol, {
				arg msg, time, addr, rxport;
				// ["CroneEngine rx command", msg, time, addr, rxport].postln;
				func.value(msg);
			}, ("/command/"++name).asSymbol);
			commands.add(cmd);
		}, {
			idx = commandNames[name];
		});
		^idx
	}

}

// dummy engine
Engine_None : CroneEngine {
	*new { arg completeFunc;
		^super.new(completeFunc);
	}

	alloc {}

	free {}
}
