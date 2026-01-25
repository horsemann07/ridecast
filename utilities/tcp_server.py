import socket
import threading

SERVER_IP = "0.0.0.0"   # Listen on all interfaces
SERVER_PORT = 3333     # Must match ESP TCP_SERVER_PORT
BUFFER_SIZE = 1024


def handle_client(conn, addr):
    print(f"[+] Client connected: {addr}")

    try:
        while True:
            data = conn.recv(BUFFER_SIZE)
            if not data:
                print(f"[-] Client disconnected: {addr}")
                break

            print(f"[RX] {addr}: {data.decode(errors='ignore')}")
            
            # Echo back
            conn.sendall(data)

    except Exception as e:
        print(f"[!] Error with {addr}: {e}")

    finally:
        conn.close()
        print(f"[x] Connection closed: {addr}")


def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    server.bind((SERVER_IP, SERVER_PORT))
    server.listen(5)

    print(f"[✓] TCP Server listening on {SERVER_IP}:{SERVER_PORT}")

    while True:
        conn, addr = server.accept()
        thread = threading.Thread(
            target=handle_client,
            args=(conn, addr),
            daemon=True
        )
        thread.start()


if __name__ == "__main__":
    start_server()
