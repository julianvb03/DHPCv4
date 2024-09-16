# Address Resolution

## ARP

- When a packet is sent at the network layer, it is encapsulated in a data link frame. The sender must know which address to place in the frame for proper communication, and this is usually the MAC address, which is where ARP comes into play.
- ARP messages are not IP packets, so they have a different Ethertype: **0x0800** for IP and **0x0806** for ARP.

## Local case

- When a device in the local network wants to send a ping, for example, the IP packet is first created. Then a bitwise AND operation is performed with the subnet mask to verify if the device with that IP is in the same subnet. If it is, it gets encapsulated in an Ethernet frame. To do this, the MAC address is needed, and this is where ARP comes in.
- A table (ARP Cache) is constructed in the host, which maps IPs to MAC addresses and starts out empty. An ARP Request is sent to a broadcast address asking for the recipient. One of the devices in the network will reply (the one with the requested IP). The responding device stores the IP and MAC address of the requester in its ARP Cache. The device with that IP will then respond with an ARP Reply in unicast mode since it now knows the requester's data. Upon receiving this, the requester updates its ARP Cache with the information of the queried device.
- In some cases, other devices in the network may also update their ARP Cache when receiving the ARP Reply.

## Remote case

- If after comparing the subnet mask with the destination IP, it's determined that the device is not in the local network, it won’t ask for the destination device's IP, but rather for the **default gateway**'s IP, which is the router that will forward the message.
- The router will store the requester's IP and MAC address in its ARP Cache and send an ARP Reply. The requester will store the router's IP and MAC address in its ARP Cache and will then send the ping to the target destination through the router. The router will forward the packet to the internet and return the response to the requester.
- In this process, the device that initiated the ping will use the external destination's IP but the MAC address of its router (which is in its ARP Cache). This MAC address will change as the packet passes through intermediate routers in different networks.

- The ARP message repeats the source and destination MAC information (even though it’s already in the Ethernet frame) because the layered model mandates that each layer must be self-sufficient.

## ARP Cache table

- The ARP Cache table removes an entry if it is not used within 15 to 20 minutes, depending on the implementation. This means it must be repopulated if new communication occurs. The ARP table is indexed by IPs, meaning that multiple IPs can map to the same MAC address, but an IP cannot map to more than one MAC address.
- If a new MAC address is received for an existing IP, the old MAC address is immediately replaced with the new one.
- It's important to note that the size of the ARP table may be limited by the operating system configuration, which could cause issues in networks with many devices.

## Reverse Address Resolution

- Reverse Address Resolution is the opposite of ARP (Address Resolution Protocol). ARP takes an IP address and returns the associated MAC address, while RARP (Reverse Address Resolution Protocol) does the opposite: by knowing the MAC address, it retrieves an IP address.
- RARP relies on a **server** that stores the MAC → IP mappings. There are three protocols that assist in this: **RARP**, **BOOTP**, and **DHCP**.

## RARP

> 💡 This protocol is obsolete, so we won’t go into much detail.

- The message format is the same as ARP.
- To query an IP, a RARP Request is broadcast, and a RARP Reply is received if the server finds the MAC in its tables, sent back in unicast mode.
- It uses a different Ethertype than ARP to facilitate filtering and to distinguish a RARP Request from an ARP Request.
- Problems:
    - It only returns IP addresses and does not configure subnet masks, limiting its usefulness.
    - It is not routable, confining it to a local network (just like ARP).

## BOOTP

- Its name comes from the need to use diskless machines.
- BOOTP asks a server for the full IP configuration, including subnet masks.
- Since it is an IP-based protocol, it can travel across routers, so the server doesn’t need to be in the local network.
- An **agent** in the local network (usually the router) receives the BOOTP Request sent as a broadcast and forwards it as unicast to the BOOTP server, which knows the server’s address.

### BOOTP without modifying ARP Cache

- The client sends a BOOTP Request broadcast to the address 255.255.255.255 with the source IP as 0.0.0.0 (since it doesn’t know its IP).
- The server responds via broadcast since the client has no ARP entry for identification.

### BOOTP can modify ARP Cache

- If allowed by the kernel, BOOTP can modify the ARP Cache by adding the client’s MAC and IP, allowing a BOOTP Reply to be sent via unicast.

### BOOTP with remote server

- The LAN sends the BOOTP Request as broadcast. The agent (router) forwards the request as unicast to the remote server and receives the unicast reply. Depending on the agent, the reply can be sent via broadcast or unicast.

## DHCP

- DHCP is similar to BOOTP but differs in how IP addresses are assigned. In DHCP, there are three options:
    - **Static configuration:** Same as BOOTP, manual IP assignment.
    - **Dynamic configuration:** IPs are leased for a period and must be renewed.
    - **Automatic configuration:** An IP is permanently assigned to a requesting MAC.
- DHCP and BOOTP packets are identical. Responses can be sent via broadcast or unicast, and a DHCP server using manual addressing is equivalent to a BOOTP server.
- It is common to leave a subset of IP ranges reserved for manual configurations.

## Security considerations

- ARP is susceptible to attacks like **ARP spoofing**, where an attacker deceives devices on the network into associating their MAC address with another device's IP. This can allow **Man-in-the-Middle (MITM)** attacks. Implementing security measures such as ARP authentication or static ARP tables in sensitive networks is crucial.
