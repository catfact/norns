// an audio "engine."
// maintains some DSP processing and provides control over parameters and analysis results
// new engines should inherit from this
NornsEngine {

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
		// this method is called in a Routine, so Server.sync can be used
	}

	free {
		// subclass responsibility to de-allocate server resources.
		// this method is called in a Routine, so Server.sync can be used
	}

	addPoll { arg name, func, periodic=true;
		name = name.asSymbol;
		NornsPollRegistry.register(name, func, periodic:periodic);
		pollNames.add(name);
		^NornsPollRegistry.getPollFromName(name);
	}

	// deinit is called in a routine
	deinit {
		arg completeFunc; 
		postln("NornsEngine.free");
		commands.do({ arg com;
			com.oscdef.free;
		});
		pollNames.do({ arg name;
			NornsPollRegistry.remove(name);
		});
		// subclass responsibility to implement free
		this.free;
		Norns.server.sync;
		completeFunc.value(this);
	}


	addCommand { arg name, format, func;
		var idx, cmd;
		name = name.asSymbol;
		postln([ "NornsEngine adding command", name, format, func ]);
		if(commandNames[name].isNil, {
			idx = commandNames.size;
			commandNames[name] = idx;
			cmd = Event.new;
			cmd.name = name;
			cmd.format = format;
			cmd.oscdef = OSCdef(name.asSymbol, {
				arg msg, time, addr, rxport;
				// ["NornsEngine rx command", msg, time, addr, rxport].postln;
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
Engine_None : NornsEngine {
	*new { arg completeFunc;
		^super.new(completeFunc);
	}

	alloc {}

	free {}
}
