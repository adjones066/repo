# Addison and Ryan - COS 331 project

## Description

This project's goal is for designing a Reliable File Transfer protocol built upon UDP. 

## Specification

### Handling of lost or out of sequence packets
Our RDT protocol will implement a variety of tools to ensure reliable data transfer.

Examples of these tools include:
 - Sequence Numbers
 - Acknowledgement packets
 - Timeout
 - Go-Back-N

This protocol will be able to respond to events such as a lost packet, a lost ACK, or a premature timeout
in a way that will preserve the integrity of the data being transferred. The Go-Back-N protocol will be 
instrumental in overcoming these obsticles. 

**Go-Back-N**

The basic description of Go-Back-N is that it is a protocol which controls how many unacknowledged packets are in the pipeline
at one time. It does this by assigning packets sequence numbers, and keeps track of the oldest unacknowledged packet as well as
the sequence number of the next packet that hasn't been sent. The latter packet that is described cannot be sent until the former
packet is ackowledged by the receiver. The sender has a fixed window side of the number of packets it will send before receiving an
acknowledgement.

A precaution that Go-Back-N makes is that it has a timeout value for the oldest transmitted, but not yet acknowledged packet.

![Diagram of GBN running correctly](correct_GBN.pdf)

**Lost Packet**

When a packet is dropped in the pipeline, the receiver will be able to tell because the sequence number of the next
packet it gets will not be what it expects. In response to this, the receiver will discard all of the out of order packets, and 
resend the ACK of the last in order packet. If either the duplicate ACK or the timeout of the dropped packet is detected first,
the sender will resend the dropped packet, as well as all other packets that "fit" in the current window size.

![Diagram of GBN correcting for packet loss](GoBackNPacketLoss.drawio.pdf)

**Premature Timeout**

In the event of a premature timeout, meaning that a packet was successfully delievered, but the acknowledgement 
was not receieved in time, the GBN protocol will just resend every packet since the one that was not acknowledged.
This is not very efficient, but it will still ensure that every packet will made it to the receiver. The flow will 
continue as normal afterwards. 

![Diagram of GBN correcting for a premature timeout](GoBackNPremTimeout.pdf)

**Lost ACK**

Acknowledgements are what allow for the sender to send new packets over the wire, so losing one would mean that 
the next packet would never be sent. In the event that, ACK0 was dropped, but ACK1 made it to the sender, the sender would not send 
the next packet because it was expecting ACK0. What makes it so that the sender does not freeze up is the eventual timeout that
occurs when a packet's acknowledgement is never received, but instead of indicating that the packet was dropped, the acknowledgement
was lost somewhere along the way. GBN will then resend all of the packets that had been sent but not acknowledged until the ACKs come back
in the correct order. 

![Diagram of GBN correcting for a lost ACK packet](GoBackNLostACK.pdf)



### Packet Description

| Field Name | Description |
| ---------- | ----------- |
| Source Port (16 bits) | Port number at the source |
| Destination Port (16 bits) | Port number at the destination |
| Source IP (32 bits) | IP address of the sender |
| Destination IP (32 bits) | IP address of the destination |
| Sequence Number Field (32 bits) | Specifies the number that is associated with the first bit of the payload. Used in ACK number calculation |
| Flags Field (6 bits) | Contains URG, ACK, PSH, RST, SYN, and FIN flags|
| Payload (1463 bytes) | Data being sent over the wire |


![Go Back N Protocol Algorithm Link](https://www.baeldung.com/cs/networking-go-back-n-protocol)

## Build

In order to run this project you need to run the executables as follows:

`./client hostname portnumber (path to desired file) (path to where file will be saved)`

![Example of our actual command for client executeable](https://repo.cse.taylor.edu/group-work/adjones-rcostell/-/blob/master/diagrams/IMG_5825_2.pdf)

`./server portnumber`

![Example of our actual command for server executeable](https://repo.cse.taylor.edu/group-work/adjones-rcostell/-/blob/master/diagrams/servercommand.pdf)

## Test

An example of how we ran the client executeable:


The network in which we ran the programs consisted of two hosts, client and server, each of them connected to a different switch. A single router connected to the two switches allowed for port forwarding and for the client and server to communicate.

Throughout the file we placed outputs to a log file so we can see what was going on while the program was running. We used these log files to determine where the program got stuck and then get back on track. We ran three major tests, Delay, Packet Reordering, and Packet Drop.

### Delay

To test the client/server communication with extra delay we ran the command `tc qdisc add dev ens33 root netem delay 1000ms`. It did basically what we expected and everything arrived in order, just slower than usual. We saw from the log files that nothing got out of order, and the output file looked identical to the input file. 

### Reordering

To test how the programs would handle reordering, we ran the command `tc qdisc add dev ens33 root netem delay 100ms reorder 50%`. The log file showed that whenever a file arrived out of order, the client would send a NACK containing the sequence number of the packet that needs to be sent first. However, say if the packets arrived in the order: 1,3,4,5,2; then the client would send a single NACK for packet 2 when it received packet 3, and then it would send an ACK when it received packet 2 after packet 5. This would result in the GBN algorithm resending packets 2-5. The resending is ultimately unneccessary, but the client will just drop any packet it is not expecting. It does not matter if the sequence number is higher or lower than what is expected. Therefore, the final output file is identical to the file server reads from. 

### Dropped packets

To test how the programs would handle dropped packets on the network, we ran the command `tc qdisc add dev ens33 root netem loss 80%`. The log file shows that whenever a packet is dropped that the client was expecting, the client would send a NACK with the desired sequence number. We ran into an interesting bug where if the same packet that the client was expecting next got dropped twice in a row, both client and server would be stuck in a receiving state. Because we only want the client to send one NACK per missing packet, the server would not send any new packets if it wasn't receiving any new ACKs or NACKs. We solved this by resending a NACK or ACK if the client did not receive a new packet within 4 seconds. This ensured that even if the same packet gets dropped multiple times, the client will still tell the server it needs that packet. This also solves the problem that if an ACK or NACK get dropped because the server will stop sending packets if the client does not send ACKs.

![Router command settings to create a delay, loss, and reordering of packets](https://repo.cse.taylor.edu/group-work/adjones-rcostell/-/blob/master/diagrams/routersettins.pdf)


