import argparse
import socket


BUF_SIZE = 1024


# Command line arguments:
# - server address
# - port number
def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Example echo back server"
    )
    parser.add_argument(
        "srv_addr", type=str, help="Server address"
    )
    parser.add_argument(
        "port_num", type=int, help="Port number"
    )

    return parser.parse_args()


def main() -> None:
    args = _parse_args()

    # Open a socket
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    except OSError:
        print("socket() failed")
        return

    # Bind
    try:
        sock.bind((args.srv_addr, args.port_num))
    except OSError:
        print("bind() failed")
        sock.close()
        return

    print(f"Open a socket: {args.srv_addr}:{args.port_num}")

    # Listen
    try:
        sock.listen()
    except OSError as msg:
        print("listen() failed")
        sock.close()
        return

    print("Wait connection from a client...")

    # Accept
    conn, _ = sock.accept()
    if not conn:
        print("accept() failed")
        sock.close()
        return

    with conn:
        print("Connected")

        while True:
            # Receive data from a client
            data = conn.recv(BUF_SIZE)
            if not data:
                print("recv() failed")
                break

            print(f"Received: {data.decode()}")

            # Echo back to the client
            sent_size = conn.send(data)
            if len(data) != sent_size:
                print("send() failed")
                break

    sock.close()
    print("Connection closed")


if __name__ == "__main__":
    main()
