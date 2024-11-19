import os
import serial
import time

os.system("cls")

arduino = serial.Serial(port="COM5", baudrate=9600, timeout=0.1)

def send_initial_time():
    current_time = time.strftime("%H %M") # 공백으로 구분해서 보내고 아두이노에서 parseInt로 분리
    print(f"Sending : {current_time}")
    arduino.write(bytes(current_time, 'utf-8'))

    time.sleep(0.5) # 모든 바이트가 전송되었을 때 까지 기다리기
    received = arduino.readline().decode('utf-8')
    print(f"received : {received}")

while (input("init? : ") == 'y'): # 입력 하고 너무 빠르게 엔터를 누르면 안 됨
    send_initial_time()
    
arduino.close()