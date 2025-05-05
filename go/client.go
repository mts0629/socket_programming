package main

import (
	"fmt"
	"net"
)

func main() {
	port := "127.0.0.1:8080"

	conn, err := net.Dial("tcp", port)
	if err != nil {
		panic(err)
	}

	defer conn.Close()

	buf := make([]byte, 1024)

	count, err = conn.Read(buf)
	if err != nil {
		panic(err)
	}

	fmt.Println(string(buf[:count]))
}
