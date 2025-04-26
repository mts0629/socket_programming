import socket


HOST = "127.0.0.1"
PORT = 8080


def main() -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        print(f"Connect to a server {HOST}:{PORT}...")

        sock.connect((HOST, PORT))

        print("Connected")

        sock.sendall(b"Hello")

        print("Connection closed")


if __name__ == "__main__":
    main()
