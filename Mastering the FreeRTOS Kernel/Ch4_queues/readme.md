# Other important topics from this chapter, but that are not  considerable enough to demo as code, are:

## Using a Queue to Send Different Types and Lengths of Data

Previous sections have demonstrated two powerful design patterns; sending 
structures to a queue, and sending pointers to a queue. Combining those techniques 
allows a task to use a single queue to receive any data type from any data source.

## Queue Sets: Receiving From Multiple Queues

Often application designs require a single task to receive data of different sizes, 
data of different meaning, and data from different sources. The previous section 
demonstrated how this can be achieved in a neat and efficient way using a single queue 
that receives structures. However, sometimes an application’s designer is working with 
constraints that limit their design choices, necessitating the use of a separate queue 
for some data sources. For example, third party code being integrated into a design might 
assume the presence of a dedicated queue. In such cases a ‘queue set’ can be used.

Queue sets allow a task to receive data from more than one queue without the task polling 
each queue in turn to determine which, if any, contains data.

A design that uses a queue set to receive data from multiple sources is less neat, and 
less efficient, than a design that achieves the same functionality using a single queue 
that receives structures. For that reason, it is recommended that queue sets are only used 
if design constraints make their use absolutely necessary.

The following sections describe how to use a queue set by:
1. Creating a queue set.
2. Adding queues to the set.
    Semaphores can also be added to a queue set. Semaphores are described 
    later in this book.
3. Reading from the queue set to determine which queues within the set contain data.
    When a queue that is a member of a set receives data, the handle of the 
    receiving queue is sent to the queue set, and returned when a task calls 
    a function that reads from the queue set. Therefore, if a queue handle is 
    returned from a queue set then the queue referenced by the handle is known 
    to contain data, and the task can then read from the queue directly.

Note: If a queue is a member of a queue set then do not read data from the queue unless the queue’s handle has first been read from the queue set.
Queue set functionality is enabled by setting the configUSE_QUEUE_SETS compile time configuration constant to 1 in FreeRTOSConfig.h.

## Using a Queue to Create a Mailbox

There is no consensus on terminology within the embedded community, and ‘mailbox’ will mean different things in different RTOSes. In this book the term mailbox is used to refer to a queue that has a length of one. A queue may get described as a mailbox because of the way it is used in the application, rather than because it has a functional difference to a queue:

- A queue is used to send data from one task to another task, or from an interrupt service routine to a task. The sender places an item in the queue, and the receiver removes the item from the queue. The data passes through the queue from the sender to the receiver.

- A mailbox is used to hold data that can be read by any task, or any interrupt service routine. The data does not pass through the mailbox, but instead remains in the mailbox until it is overwritten. The sender overwrites the value in the mailbox. The receiver reads the value from the mailbox, but does not remove the value from the mailbox.