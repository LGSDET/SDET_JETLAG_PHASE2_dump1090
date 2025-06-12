import socket
import time

HOST = '172.26.70.4'   # dump1090가 실행 중인 서버 IP (예: '192.168.0.101')
PORT = 30001         # dump1090 RAW 입력 포트 (기본값)

# 가짜 ADS-B 메시지 예시 (DF17, 112비트)
fake_adsb_messages = [
    '*8DA6EBEE59CD8318B4107A93F56A;',
    '*8DA6EBEE9915B38BD0044623C73B;',
    '*02E19838B360D7;',
    '*5DA43BC6448996;',
    '*02E19838B360D7;',
    '*02E19838B360D7;',
]

with socket.create_connection((HOST, PORT)) as s:
    for msg in fake_adsb_messages:
        s.sendall((msg + '\n').encode('ascii'))
        print(f"Sent: {msg}")
        time.sleep(0.5)  # 0.5초 간격으로 전송