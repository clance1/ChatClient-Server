# Networks-programming3

## Contributers
Carson Lance (clance1)
Cole Pickford (cpickfor)
Jack Conway (jconway7)

## Overview
The two folders contain a client and a server respectfully. Both run with multi-threading for concurrency. The server allows for multiple connections at anytime. The client uses multithreading for the constant ability to receive a message.

## Usage

Server:
```bash
cd server
make
./chatclient PORT
```
Client:
```bash
cd client
make
./chatclient ADDR PORT USERNAME
```

## Functions
- B: Broadcast.
> Sends a message to everyone connected to the server
- P: Private Message
> Sends a message to a desired user
- H: History
> Prints the full history of chats for the user
- X: Exit
> Exits the server