## breaking changes 

we are proposing some sweeping changes to the norns supercollider interface.

these changes will break all Engines! but we think it is worth it.

this is a document of the breakages and their motivations.

### rename Crone -> Norns

within the norns, "crone" designates the audio backend environment. in norns 1.x, this environment was entirely implemented in supercollider, so `Crone` was the main interface class.

in norns 2.x, `crone` is a separate process, and the naming is now very confusing. we'll rename `Crone..` to `Norns...` in supercollider classes to make it clear that the purpose of these classes is to interface with the rest of the Norns ecosystem.

### remove CroneAudioContext

since 2.0, we are no longer managing routing and levels in supercollider, SC's input and ouputs are just the standard jack I/O ports, and amplitude polls are are implemented in the `crone` process.

so the notion of an "audio context" is obsolete: we don't need dedicated I/O busses, nor synths to patch things, nor Groups to manage the synths.

this allows us to remove a lot of code 

the one thing we lose is the builtin pitch-reporting poll, since freq-domain analysis is NYI in `crone`. but (1) this did not seem to be used at all outside of the intro "studies" scripts, and (2) it is trivial to add a pitch poll to any Engine. 

#### Engine changes required

- rename `Crone` -> `Norns`.
- `alloc { context, callback }` has changed to `alloc { callback }`.
- fields from `context` need replacing:
  - `context.server` can be replaced with `Norns.server` (or even `Server.default`? hm..)
	- instead of targeting `context.xg` (the "processing group") on Synth/Node creation, just target the server.
  - context I/O busses can be replaced with the default "hardware" I/O busess:
    - for output, `Out.ar([0,1])`
	- for input `SoundIn.ar([0,1])`, or compute the correct bus offsets for `In.ar` (see `Bus` helpfile)


# new features

there are also some non-breaking changes here, mostly aimed at reducing Engine boilerplate and offering new methods of creating Engines from runtime-interpreted sclang code instead of compiled classes.

the removal of norns-specific cruft will also 

## helper methods for class-based engines

it is a good pattern for a subclass of `NornsEngine` to be a simple interface wrapper class around a "kernel" class which does all the real work. this keeps the main functionality intependent from the norns ecosystem and more easily developed and tested on other systems.

but adding norns "commands" to the engine logic tends to create a lot of repetetive boilerplate, no matter where it is located.

if the "kernel"/"interface" structure is used, then we can generate command mappings automatically by inspecting the kernel class methods. 

a kernel method of the form `set_foo { arg value... }` shall be mapped to an engine command called `foo`, with format `"f"`, which calls `set_foo { msg[1] }`.

for polyphonic or paraphonic engines, a kernel method of the form `voice_bar { arg index, value... }` shall be mapped to an engine command called `bar`, with format `"if"`, which calls `voice_bar { msg[1], msg[2] }`.

## new "dynamic" engine creation methods (not requiring classes)

### calling into NornsEngineFactory

**TODO**

### using environment variables with naming convention

**TODO**
