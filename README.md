# ring-db

This project consisted on the implementation of a ring database.

In this database there are many nodes, each containing any number of objects. 
The nodes are connected in a way that they form a ring structure. Meaning that each node is connected via TCP to two adjacent nodes, a predecessor and a successor. These nodes are connected in numerical order.

There are three basic operations in this database:
- A node wants to know where a certain object is;
- A new node wants to enter the database saving inside the objects between him and his successor;
- A node wants to exit the database attributing his objects to other nodes.

Nodes can also be connected via a chord, an imaginary unidirectional connection, where messages are sent via UDP.
