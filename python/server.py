import socket


HOST = "127.0.0.1"
PORT = 8080
BUF_SIZE = 1024


def main() -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind((HOST, PORT))
        print(f"Open a socket: {HOST}:{PORT}")

        sock.listen()

        print("Wait connection from a client...")

        conn, _ = sock.accept()
        with conn:
            print("Connected")

            data = conn.recv(BUF_SIZE)

            print(f"Received: {data.decode()}")

        print("Connection closed")


if __name__ == "__main__":
    main()
