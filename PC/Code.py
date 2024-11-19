import os
import serial
import time

import requests
import datetime
import json

os.system("cls")

class Alarm:
    def __init__(self, arduino):
        self.arduino = arduino

    def send_initial_time(self):
        current_time = time.strftime("%H %M") # 공백으로 구분해서 보내고 아두이노에서 parseInt로 분리
        print(f"Sending : {current_time}")
        self.arduino.write(bytes(current_time, 'utf-8'))

        time.sleep(0.5) # 모든 바이트가 전송되었을 때 까지 기다리기
        received = self.arduino.readline().decode('utf-8')
        print(f"Received : {received}")

class Umbrella:
    BASE_URL = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getVilageFcst"

    def __init__(self, arduino, API_KEY, NX, NY):
        self.arduino = arduino
        self.API_KEY = self.load_api_key()
        self.NX, self.NY = NX, NY # 상도1동

    def load_api_key(self):
        script_dir = os.path.dirname(__file__)
        with open(os.path.join(script_dir, "api_key.json"), "r") as file:
            data = json.load(file)
            return data['KEY']

    def get_latest_base_time(self, now):
        forecast_times = ["0200", "0500", "0800", "1100", "1400", "1700", "2000", "2300"]
        
        for time in reversed(forecast_times):
            if now.strftime("%H%M") >= time:
                return time
            
        # 만약 현재 시간이 0200 이전이면, 가장 마지막 발표 시간인 2300으로 설정
        return forecast_times[-1]

    def will_it_rain(self):
        now = datetime.datetime.now()

        if now.hour < 5:
            base_date = (now - datetime.timedelta(days=1)).strftime("%Y%m%d")  # 어제 날짜
        else:
            base_date = now.strftime("%Y%m%d")  # 오늘 날짜
        
        base_time = self.get_latest_base_time(now)

        params = {
            "serviceKey": self.API_KEY,
            "numOfRows": "1000",
            "pageNo": "1",
            "dataType": "JSON",
            "base_date": base_date,
            "base_time": base_time,
            "nx": self.NX,
            "ny": self.NY,
        }

        try:
            response = requests.get(self.BASE_URL, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()

            try:
                items = data['response']['body']['items']['item']
                rain_prob_threshold = 50 # 기준 강수확률

                for item in items:
                    if item['category'] == 'POP':
                        fcst_time = int(item['fcstTime'])
                        fcst_prob = int(item['fcstValue'])
                        if 600 <= fcst_time <= 2100 and fcst_prob >= rain_prob_threshold:
                            return True

                return False
            except KeyError:
                raise KeyError("JSON 데이터 구조 오류")
        except requests.exceptions.RequestException as e:
            raise ConnectionError("네트워크 또는 API 요청 오류:", e)
        except ValueError:
            raise ValueError("JSON 응답이 비어있거나 형식이 잘못되었습니다.")

def main():
    arduino = serial.Serial(port="COM5", baudrate=9600, timeout=0.1)

    alarm = Alarm(arduino)
    umbrella = Umbrella(arduino=arduino, API_KEY = "make your own ./api_key.json", NX=59, NY=125)

    if (input("Enter 'y' after setting up Arduino: ") == 'y'): # 입력 하고 너무 빠르게 엔터를 누르면 안 됨
        alarm.send_initial_time()

    if (input("Enter 'y' to check the weather: ") == 'y'):
        try:
            if umbrella.will_it_rain():
                print("비가 내릴 수 있습니다.")
            else:
                print("비가 내리지 않을 것입니다.")
        except Exception as e:
            print(f"error occured: {e}")

if __name__ == "__main__":
    main()