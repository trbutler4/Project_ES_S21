from requests import Request, Session
from requests.exceptions import ConnectionError, Timeout, TooManyRedirects
import json
import time
import serial 


###################
# API stuff

# real data
API_KEY = 'e4e16906-0114-410b-85c3-9f7a978e0500'
API_URL = 'https://pro-api.coinmarketcap.com/v1/cryptocurrency/listings/latest'

# sandbox data
TEST_KEY = 'b54bcf4d-1bca-4e8e-9a24-22ff2c3d462c'
TEST_URL = 'https://sandbox-api.coinmarketcap.com/v1/cryptocurrency/listings/latest'

parameters = {
  'start':'1',
  'limit':'5000',
  'convert':'USD'
}
headers = {
  'Accepts': 'application/json',
  'X-CMC_PRO_API_KEY': API_KEY,  # change to API_KEY for real data
}

session = Session()
session.headers.update(headers)

def getData():
  try:
    response = session.get(API_URL, params=parameters) # change to API_URL for real data
    data = json.loads(response.text)
    return data
  except (ConnectionError, Timeout, TooManyRedirects) as e:
    print(e)

#BTC_data = data['data'][0] # Bitcoin data
#ETH_data = data['data'][1] # Ethereum data

#BTC_price = BTC_data['quote']['USD']['price'] # get BTC price
#ETH_price = ETH_data['quote']['USD']['price'] # get ETH price


#print('BTC: ', BTC_price, '\n') # print BTC price
#print('ETH: ', ETH_price, '\n') # print ETH price
###################

##################
# Serial stuff
arduino = serial.Serial(port = 'COM4', baudrate=9600, timeout = .1)
def write(x):
    arduino.write(bytes(x, 'utf-8'))
    time.sleep(0.05)

#####################


def formatString():
  data = getData()
  toSend = ''
  for i in range(0,2):
    price = data['data'][i]['quote']['USD']['price']
    toSend = toSend + str(round(price, 2)) + ','
  toSend = toSend + '\n'
  return toSend


while True:
 write(formatString())
 time.sleep(5)








