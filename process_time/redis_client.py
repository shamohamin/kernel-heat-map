import redis
import random 

keys = ["A", "B", "C", "D"]
client = redis.Redis(host='localhost', port=6379, db=0)

while True: 
    client.set(random.choice(keys), random.randint(1, 100))
    print(client.get(random.choice(keys)))
