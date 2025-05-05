package main

import (
	"net"
)

func main() {
	port := "127.0.0.1:8080"

	listener, err := net.Listen("tcp", port)
	if err != nil {
		panic(err)
	}

	for {
		conn, err := listener.Accept()

		if err != nil {
			continue
		} else {
			go func() {
				defer conn.Close()

				conn.Write([]byte("hello"))
			}()
		}
	}
}
