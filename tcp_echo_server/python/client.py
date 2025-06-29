import argparse
import socket


BUF_SIZE = 1024


# Command line arguments:
# - server address
# - port number
def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Example echo back client"
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

    # Connect to a server
    try:
        print(f"Connect to a server {args.srv_addr}:{args.port_num}...")
        sock.connect((args.srv_addr, args.port_num))
    except OSError as msg:
        print("connect() failed")
        sock.close()
        return

    print("Connected")

    while True:
        # Input string
        send_str = input("Sending > ")
        if len(send_str) > BUF_SIZE:
            print(f"String is too long: length must be <= {BUF_SIZE}")
            continue

        # Send the string to the server
        sent_size = sock.send(send_str.encode())
        if len(send_str) != sent_size:
            print("send() failed")
            break

        # Receive a echo-backed string from the server
        data = sock.recv(BUF_SIZE)
        if not data:
            print("recv() failed")
            break

        print(f"Received: {data.decode()}")

    sock.close()
    print("Connection closed")


if __name__ == "__main__":
    main()
