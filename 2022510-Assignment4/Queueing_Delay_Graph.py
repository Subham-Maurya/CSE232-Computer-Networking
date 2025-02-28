import matplotlib.pyplot as plt

# Path to the trace file
trace_file_path = "/home/subham-maurya/ns-allinone-3.42/ns-3.42/tcp-example.tr"

# Dictionaries to store enqueue and dequeue times
enqueue_times = {}
dequeue_times = {}
queueing_delays = []

with open(trace_file_path, "r") as file:
    for line in file:
        # Split the line into parts
        parts = line.strip().split()
        
        # Ensure it's a valid line with enough columns
        if len(parts) < 8:
            continue
        
        # Extract event type (+ for enqueue, - for dequeue), time, and packet ID
        event_type = parts[0]
        time = float(parts[1])
        packet_id = parts[-1]  # packet ID is the last field
        
        if event_type == "+":
            enqueue_times[packet_id] = time
        elif event_type == "-":
            if packet_id in enqueue_times:
                dequeue_times[packet_id] = time
                delay = time - enqueue_times[packet_id]
                queueing_delays.append((time, delay))  # Store dequeue time and delay

# Sort delays by time for plotting
queueing_delays.sort()

# Separate time and delay into two lists for plotting
times = [x[0] for x in queueing_delays]
delays = [x[1] for x in queueing_delays]

# Plot Queueing Delay vs Time
plt.figure(figsize=(10, 6))
plt.plot(times, delays, label="Queueing Delay", color='green', linewidth=1.5)
plt.title("Queueing Delay vs Time", fontsize=14)
plt.xlabel("Time (in seconds)", fontsize=12)
plt.ylabel("Queueing Delay (in seconds)", fontsize=12)
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend(fontsize=12)
plt.tight_layout()
plt.show()

