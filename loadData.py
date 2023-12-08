import json

with open("memory_allocator.json", "r") as file:
    data = json.load(file)


memcachedData = {}
for key in data.keys():
    if data[key]["process_name"] == "memcached":
        memcachedData[key] = data[key]
    # print(key)

for pid in memcachedData.keys():
    for tid in memcachedData[pid].keys():
        if tid == "process_name": continue
        # print(memcachedData[pid][tid])
        # print( memcachedData[pid][tid]["0"]["allocation_ts"])
        # memcachedData[pid][tid]["0"]["aggregated"] = []
        # memcachedData[pid][tid]["0"]["aggregated"] = []
        

        for k in ["0", "1"]:
            l = [] 
            for i in range(len(memcachedData[pid][tid][k]["allocation_ts"])):
                size = memcachedData[pid][tid][k]["allocated_size"][i]
                t = memcachedData[pid][tid][k]["allocation_ts"][i]
                s = f"time: {t} - allocation_size: {size}"
                l.append(s)
                # l.append([ , ])
            del memcachedData[pid][tid][k]["allocation_ts"]
            del memcachedData[pid][tid][k]["allocated_size"]

            memcachedData[pid][tid][k]["aggregated"] = l
        # memcachedData[pid][tid]["1"]["aggregated"] = [*memcachedData[pid][tid]["1"]["allocation_ts"], *memcachedData[pid][tid]["1"]["allocated_size"]]

with open("memcached.json", "w") as file:
    json.dump(memcachedData, file)
# print(memcachedData)