# Structure

This file shall explain some of the names and classes used in this library.

It was taken care to provide the core functionality on a C language level. The
full functionality should be usable inside microcontroller without dynamic
memory allocation. For more powerful POSIX systems a set of c++ wrapper classes
where implemented to provide a robust and flexible API. The main loop does
polling on non-blocking IO read-functions. This is *hugely* inefficient but
allows to provide a very simple structure, which can be easily reused in
microcontroller.

## Concepts in C

The parts written in C heavily use the linked lists contained in
include/ndlcom/list.h which was copied from the [Linux
kernel](https://github.com/torvalds/linux/blob/master/include/linux/list.h).
This allows to achieve similar effects as in "dynamic" data structures without
having to allocate memory. Each entry in a linked list is stored directly inside the
struct of the connected entity. See NDLComExternalInterface::list_head for
example.

## Concepts in C++

The main idea in the C++ wrapper classes is to have exactly one entity
(ndlcom::Bridge) which owns all dependent objects via
[std::shared_ptr](http://en.cppreference.com/w/cpp/memory/shared_ptr) kept in
private variables like ndlcom::Bridge::bridgeHandler. Only
[std::weak_ptr](http://en.cppreference.com/w/cpp/memory/weak_ptr) are exported
to an user of the API to retain full control over the lifetime of objects.
Factory functions like ndlcom::Bridge::createNode and
ndlcom::Bridge::destroyNode are used to add and remove handlers in a safe way.
Thus, dynamic memory allocation is used heavily!

# Mapping between C and C++

C++ wrapper classes are provided for the important, user facing, pieces of the
API. The mapping is explained here, from a height of 10000ft.

| C | C++ | explanation |
| --- | --- | --- |
| struct NDLComBridge | class ndlcom::Bridge | The main entity. Orchestrates reading and writing of encoded messages from NDLComExternalInterface and passes decoded messages to registered NDLComBridgeHandler |
| struct NDLComExternalInterface | class ndlcom::ExternalInterfaceBase | Represents an actual hardware interface, has to provide a NDLComExternalInterface::read() and NDLComExternalInterface::write() in a non-blocking way |
| struct NDLComBridgeHandler | class ndlcom::BridgeHandler | The internal side of the bridge, whose handler is called for _every_ passing packet |
| struct NDLComNode | class ndlcom::Node | A special NDLComBridgeHandler which filters all passing messages and only acts on messages directed at its own deviceId. Can own a number of NDLComNodeHandler and calls their handle for every packet directed at the Node |
| struct NDLComNodeHandler | class ndlcom::NodeHandler | Is owned by an NDLComNode, and its NDLComNodeHandler::handler is called for every message directed at this NDLComNode |

