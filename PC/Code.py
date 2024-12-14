import os
import serial
import time

import requests
import datetime
import json

import asyncio

os.system("cls")

class Alarm:
    def __init__(self, arduino):
        self.arduino = arduino

    def send_initial_time(self):
        current_time = time.strftime("%H %M") # 공백으로 구분해서 보내고 아두이노에서 parseInt로 분리
        print(f"Sending : {current_time}")
        self.arduino.write(bytes(current_time, 'utf-8'))

        time.sleep(2) # *중요: 모든 바이트가 전송되었을 때 까지 기다리기
        if (self.arduino.readable()):
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
        forecast_times = ["0200", "0500", "0800", "1100", "1400", "1700", "2000", "2300"] # 발표 시간. API 제공 시간은 10분 뒤
        
        for time in reversed(forecast_times):
            if (now - datetime.timedelta(minutes=10)).strftime("%H%M") >= time: # 현재 시간보다 10분 전인 시간을 기준으로
                return time
            
        # 만약 현재 시간이 0200 이전이면, 가장 마지막 발표 시간인 2300으로 설정
        return forecast_times[-1]

    def will_it_rain(self, rain_prob_threshold=50):
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
            res = response.json()

            try:
                data = res['response']['body']['items']['item']

                flag = False

                for item in data:
                    if item['category'] == 'POP':
                        fcst_date = int(item['fcstDate'])
                        fcst_time = int(item['fcstTime'])
                        fcst_prob = int(item['fcstValue'])

                        if fcst_date != int(base_date) or fcst_time < now.hour * 100: # 3일치 데이터가 나오기 때문에 오늘 날짜만 필터링, 현재 시간 이전의 데이터도 건너뛰기
                            continue
                        elif 600 <= fcst_time <= 2100 and fcst_prob >= rain_prob_threshold:
                            flag = True

                        print(f"날짜: {fcst_date}    시간: {fcst_time}    강수확률: {fcst_prob} %")

                return flag
            except KeyError:
                raise KeyError("JSON 데이터 구조 오류")
        except requests.exceptions.RequestException as e:
            raise ConnectionError("네트워크 또는 API 요청 오류:", e)
        except ValueError:
            raise ValueError("JSON 응답이 비어있거나 형식이 잘못되었습니다.")
        
    async def send_rainable(self, hour, minute, interval=60):
        while True:
            now = datetime.datetime.now()
            if now.hour == hour and now.minute == minute:
                self.arduino.write(bytes("1" if self.will_it_rain() else "0", 'utf-8'))
            await asyncio.sleep(interval)

def main():
    arduino = serial.Serial(port="COM5", baudrate=9600, timeout=0.1)

    alarm = Alarm(arduino=arduino)
    umbrella = Umbrella(arduino=arduino, API_KEY = "make your own ./api_key.json", NX=59, NY=125)

    if (input("Enter 'y' after setting up Arduino: ") == 'y'): # 입력 하고 너무 빠르게 엔터를 누르면 안 됨
        alarm.send_initial_time()

    # asyncio.run(umbrella.send_rainable())
    umbrella.will_it_rain()
    
    arduino.close()

if __name__ == "__main__":
    main()