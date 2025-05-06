package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"strings"
)

func main() {
	port := "127.0.0.1:8080"

	// Socket and connect
	fmt.Println("Connect to a server", port)
	conn, err := net.Dial("tcp", port)
	if err != nil {
		fmt.Println("connect() failed")
		return
	}

	fmt.Println("Connected")

	defer conn.Close()

	buf_size := 1024

	for {
		// Input string
		fmt.Print("Sending > ")
		line, err := bufio.NewReader(os.Stdin).ReadString('\n')
		if err != nil {
			fmt.Println("Reading stdin failed, please retry:")
			continue
		}
		line = strings.TrimSuffix(line, "\n") // Trim a line feed

		// Send the string to the server
		_, err = conn.Write([]byte(line))
		if err != nil {
			fmt.Println("write() failed")
			break
		}

		// Receive a echo-backed string from the server
		buf := make([]byte, buf_size)
		count, err := conn.Read(buf)
		if err != nil {
			fmt.Println("read() failed")
			break
		}

		fmt.Println("Received:", string(buf[:count]))
	}

	fmt.Println("Connection closed")
}
