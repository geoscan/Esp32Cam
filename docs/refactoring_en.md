# Refactoring guidelines / milestones

## System layer, general concerns

Severe mistakes were made from the architectural point of view. The code is
detached from neither ESP-IDF framework, nor the hardware. Moreover, it
comprises multiple parts intertwined at various levels of abstraction.

The ideal situation: the application code must invoke system level code, which,
in its turn, must depend on the actual implementation.

Here is the list of doable things that might be taken care of, so the
aforementioned goal will become closer through series of incremental steps;

- [ ] `utility`: Extract out system-related stuff into a different component
	- System-level, and middleware components (such as UartDevice) must reside in a separate component.

## Overengineering

- [ ] Make `module` API and field paradigm separate
	- `module` API implements the paradigm of virtual twins of physical or logical "devices" configurable through **fields**
	- It greatly increases the amount of code that has to be written, and obfuscates the code, while providing no substantial practical benefits;
	- Better be replaced with a more straightforward approach;
		- Invoke modules directly (`Camera`, `Wi-Fi`, ...);
			- not necessarily in a single component;
		- Implement fields later, if needed
- RR library abuse
	- It found its uses in this code beyond event distribution;
	- [ ] Refactor legacy topics
	- [ ] Add a layer of abstraction over RR
	- [ ] XXX? Move subscription topics from RR closer to the actual modules that produce the events

## Other

- `wifi_uart_bridge`
	- serves as an inter-protocol router;
	- enables one to add runtime reduction rules to foward, for example, packages from UART to TCP;
	- things to fix
		- [ ] clunky API: less callbacks,
		- [ ] complicated message delivery
			- now
				- get a request from an endpoint
				- apply a reduction rule
				- notify subscribers through callbacks
			- what would be great
				- get a request from an endpoing
				- apply a reduction rule
				- push the buffer into a working queue for notification
				- suspend the notifier's thread
					- unless called from the context of the working queue's thread
				- deliver the message through notification queue
				- check if there are any responses
				- if there're no any, resume the notifier's thread
				- in a nutshell
					- just plain simple working queue
