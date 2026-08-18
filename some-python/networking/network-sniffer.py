import socket
import struct

def format_mac(bytes_addr):
	return ":".join(f'{b:02x}' for b in bytes_addr)

def format_ip(bytes_addr):
	return '.'.join(str(b) for b in bytes_addr)

def main() :
	raw_socket = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x0003))

	print("Listening for raw ethernet frames...")

	try:
		while True:
			raw_data, _ = raw_socket.recvfrom(65535)
			dest_mac, src_mac, eth_proto = struct.unpack('! 6s 6s H', raw_data[:14])
			
			print(f"\n[Ethernet Frame]")
			print(f" Source MAC: {format_mac(src_mac)} -> Dest MAC: {format_mac(dest_mac)} | Protocol: {hex(eth_proto)}")

			if eth_proto == 0x0800:
				ip_header = raw_data[14:34]
				version_ihl, tos, total_length, pkt_id, flags_offset, ttl, proto, checksum, src_ip, dest_ip = struct.unpack('! B B H H H B B H 4s 4s', ip_header)
				ihl = (version_ihl & 0x0F) *4

				print(f" [IPv4 Packet]")
				print(f" Source IP: {format_ip(src_ip)} -> Dest IP: {format_ip(dest_ip)} | Protocol: {proto} | TTL: {ttl}")
				
				if proto == 6:
					tcp_start = 14 + ihl
					tcp_header = raw_data[tcp_start:tcp_start + 20]
					
					src_port, dest_port, seq, ack, offset_reserved_flags, flags = struct.unpack('! H H L L B B', tcp_heade[:14])
					flag_syn = (flags & 0x02) >> 1
					flag_ack = (flags & 0x10) >> 4
					flag_fin = (flags & 0x01)
					print(f"[TCP Segment]")
					print(f"Source Port: {src_port} -> Dest Port: {dest_port}")
					print(f"Sequence: {seq} | Ack: {ack}")
					print(f"Flags -> SYN: {flag_syn} | ACK: {flag_ack} | FIN: {flag_fin}")

	except KeyboardInterrupt:
			print("\n Stopping sniffer")
			raw_socket.close()

if __name__ == "__main__":
	main()
