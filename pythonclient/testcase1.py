import socket
import struct

# Define action codes
READ_COILS = 0
READ_DIGITALS = 1
READ_REGISTERS = 2
READ_HOLDING = 3
WRITE_COILS = 4
WRITE_DIGITALS = 5
WRITE_REGISTERS = 6
WRITE_HOLDING = 7

print("starting test case 1")
# Server address and port
server_address = ('localhost', 5030)

def send_request(action_code, byte_count, address, data=None):
    # Create a TCP/IP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        # Connect to the server
        client_socket.connect(server_address)
        # Pack the request using struct.pack
        request = struct.pack('!HHH', action_code, byte_count, address)
         # If it's a write operation, include data
        if data:
            request += struct.pack(f'!{len(data)}s', data)
        # Send the request
        client_socket.sendall(request)
        print("debug step 60")
        # Receive the response
        response = client_socket.recv(1024)
        print("debug step 70")
    return response

# Example usage:
# Reading from address 0
print("debug step 10") 

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
    # Connect to the server
    client_socket.connect(server_address)
    #client_socket.sendall(b'\x00\x05\x00\x01\x00\x02\xFF')
    #client_socket.sendall(b'\x00\x05\x00\x07\x01\x30\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF')
    client_socket.sendall(b'\x00\x05\x00\x07\x01\x30\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF')
    read_response = client_socket.recv(1024)
print(f'Read Response: {read_response}')

#read_response = send_request(b'\x01\x02', b'\x03\x04', b'\x05\x06')
#print(f'Read Response: {read_response}')

read_response = send_request(READ_REGISTERS, 30, 1)
print(f'Read Response: {read_response}')

# Writing to address 0 with data b'\x01\x02\x03'
write_response = send_request(WRITE_REGISTERS, len(b'\x01\x02\x03'), 0, b'\x01\x02\x03')
print(f'Write Response: {write_response}')