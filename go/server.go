package main

import (
	"fmt"
	"net"
)

func main() {
	port := "127.0.0.1:8080"

	// Socket, bind and listen
	listener, err := net.Listen("tcp", port)
	if err != nil {
		fmt.Println("listen() failed")
		return
	}

	fmt.Println("Open a socket:", port)
	fmt.Println("Wait connection from a client...")

	// Accept
	conn, err := listener.Accept()
	if err != nil {
		fmt.Println("accept() failed")
		return
	}

	fmt.Println("Connected")

	defer conn.Close()

	buf_size := 1024

	for {
		buf := make([]byte, buf_size)

		// Receive data from a client
		count, err := conn.Read(buf)
		if err != nil {
			fmt.Println("read() failed")
			break
		}

		fmt.Println("Received:", string(buf[:count]))

		// Echo back to the client
		_, err = conn.Write(buf)
		if err != nil {
			fmt.Println("write() failed")
			break
		}
	}

	fmt.Println("Connection closed")
}
