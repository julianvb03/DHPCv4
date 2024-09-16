# Problem Domain Document

## 1. Objective

The main objective of this project is to develop a functional **DHCP server** and a **DHCP client**. The server must be able to handle requests from any client connected to the network, dynamically assigning IP addresses and necessary network parameters. The client must be able to request an IP address from the server and communicate with it to obtain, renew, or release network configurations as needed.

## 2. Introduction

### Server Responsibilities:
- The DHCP server is responsible for listening to client requests and dynamically assigning network configurations such as IP addresses, subnet masks, and default gateways.
- It manages a pool of IP addresses and handles the **lease duration** of IP assignments, allowing clients to renew or release their addresses when needed.
- The server must ensure it handles concurrent requests from multiple clients.

### Client Responsibilities:
- The DHCP client initiates communication by requesting network configuration details, such as an IP address, from the server.
- The client must be able to interact with the server to receive an IP address and manage IP renewals or releases as needed.
- It must handle the sending and receiving of **DHCP** messages like **DISCOVER, REQUEST**, and **RELEASE**.

### Agent Configuration in Case of Remote Connection:
In the case of a **remote connection**, both the server and the client must be configured to handle the connection through a **DHCP relay agent**. This involves the following:
- **Client:** The client must be configured to send DHCPDISCOVER requests to a remote relay agent, which then redirects the request to the DHCP server.
- **Server:** The server must be able to accept requests coming from a relay agent, process them as if they were directly from the client, and return the response through the relay agent.

The **DHCP relay agent** is necessary to connect clients located in different subnets from the DHCP server. This agent must forward DHCP packets between the client and the server, ensuring that the responses reach the client correctly through the remote network.

## 3. DHCP Client Requirements

The client must meet the following requirements:

### Functional Requirements:
1. **Choice of Language:** The client can be developed in any programming language that supports the Sockets API (e.g., Python, Java, C).
2. **DHCP Message Handling:** The client must handle the main phases of the DHCP process:
   - **DHCPDISCOVER:** Send a **DHCPDISCOVER** message to the server to request an IP address.
   - **DHCPOFFER:** Receive an IP address offer from the server via a **DHCPOFFER** message.
   - **DHCPREQUEST:** Confirm the intent to lease the offered IP by sending a **DHCPREQUEST** message to the server.
   - **DHCPACK:** Receive the final confirmation of the IP assignment from the server via a **DHCPACK** message.
   - (Optional) **DHCPNAK:** Handle a **DHCPNAK** if the server cannot grant the requested IP.
   - (Optional) **DHCPDECLINE:** Send a **DHCPDECLINE** message to the server if the client detects that the offered IP is in use.
   - **DHCPRELEASE:** Send a **DHCPRELEASE** message to the server to release the assigned IP when it is no longer needed.
3. **Display Network Configuration:** The client must display the assigned network configurations after receiving the IP, such as:
   - IP address
   - Subnet mask
   - Default gateway
   - DNS server
   - Lease duration (if available)
4. **IP Address Renewal:** The client must be able to renew the IP address before it expires by sending a **DHCPREQUEST** message to the server.
5. **IP Address Release:** The client must release the IP when it is no longer needed by sending a **DHCPRELEASE** message.
6. **Error Handling:** The client must handle errors such as not receiving a response within a time limit or receiving incorrect messages from the server.

## 4. DHCP Server Requirements

The server must meet the following requirements:

### Functional Requirements:
1. **Language Restriction:** The server must be implemented in C, using the Berkeley Sockets API.
2. **Listening for DHCP Requests:** The server must listen on port 67 to receive **DHCPDISCOVER** messages from clients.
3. **Dynamic IP Address Assignment:** The server must manage a pool of available IP addresses to assign to clients.
4. **DHCP Message Handling:** The server must handle the main phases of the DHCP process:
   - **DHCPDISCOVER:** Respond to the client with a **DHCPOFFER** message, offering an IP address.
   - **DHCPREQUEST:** Confirm the IP assignment by sending a **DHCPACK** message to the client.
   - (Optional) **DHCPNAK:** Send a **DHCPNAK** if the requested IP cannot be granted.
   - (Optional) **DHCPDECLINE:** Handle cases where the client rejects an offered IP.
   - **DHCPRELEASE:** Process **DHCPRELEASE** messages from the client to return the IP to the pool.
5. **Lease Management:** The server must track the lease duration of each assigned IP address, allowing clients to renew their leases or release the IPs.
6. **Concurrency:** The server must handle multiple client requests simultaneously using multithreading.
7. **Event Logging:** The server must log each IP assignment, renewal, release, and error to facilitate debugging and improve system efficiency, as well as track which device was assigned each IP over time.

## 5. Client-Server Communication Overview

Both the **client** and **server** must communicate using **UDP** as the transport protocol. The standard ports used are:
- **DHCP Server Port:** 67
- **DHCP Client Port:** 68

The **client** initiates communication by sending a broadcast message (**DHCPDISCOVER**) to the network. The server responds with a **DHCPOFFER** message containing an available IP address. The client then requests the IP by sending a **DHCPREQUEST** message, and the server confirms it with a **DHCPACK** message. Both applications must correctly handle this sequence.

### Message Flow:
1. **DHCPDISCOVER:** Client -> Server (Request for IP address)
2. **DHCPOFFER:** Server -> Client (Offer of IP address)
3. **DHCPREQUEST:** Client -> Server (Request to lease the IP)
4. **DHCPACK:** Server -> Client (Confirmation of lease)

(Optional):
- **DHCPRELEASE:** Client -> Server (Release of IP)
- **DHCPDECLINE:** Client -> Server (Rejection of IP)
- **DHCPNAK:** Server -> Client (Server cannot fulfill request)
