# Wi-Fi-UART bridge

Runtime software routing of messages between interfaces.

The set of responsibilities for this module goes beyond bridging Wi-Fi and
UART. The rationale of this module is to provide a way to minimize efforts when
altering routing rules. Say, in one build configuration, a package from TCP
must be processed by MAVLink, while in the other -- it must go directly to
UART, and, once again, depending on the configuration, either be or not to be
mirrored to UDP.

There are some key ideas to get conceptualized:

- Each receiver is associated with its `EndpointVariant` instance, which serves
  as an identifier;
- When performing routing, a match/reduce scheme is employed:

```pseudocode
fn reduce(originator: EndpointVariant, message) {
    for acceptor in get_acceptors() {
        reduction = try_reduce(originator, acceptor);

        if reduction {
            acceptor.accept(originator, message);
        }
    }
}
```

- Reduction rules may be added or removed;
- During a reduction iteration, a single buffer is used.
