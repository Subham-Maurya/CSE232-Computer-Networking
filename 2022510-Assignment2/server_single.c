#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define PROC_DIR "/proc"
#define STAT_FILE_LEN 512

// Structure to store client information
typedef struct {
    int client_id;
    int socket;
} client_info;

// Structure to hold process information
typedef struct {
    char name[256];
    int pid;
    unsigned long long cpu_time;  // Total CPU time (user + kernel)
} process_info;

int parse_stat_file(char *pid_dir, process_info *proc_info) {
    char stat_path[STAT_FILE_LEN];
    snprintf(stat_path, sizeof(stat_path), "%s/stat", pid_dir);

    FILE *file = fopen(stat_path, "r");
    if (!file) {
        return -1;
    }

    // Read the /proc/[pid]/stat file to extract the necessary information
    int pid;
    char comm[256];
    unsigned long utime, stime;

    // Read the required fields (comm is the name, pid is process ID, utime and stime are CPU times)
    fscanf(file, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %lu %lu", &pid, comm, &utime, &stime);
    fclose(file);

    proc_info->pid = pid;
    snprintf(proc_info->name, sizeof(proc_info->name), "%s", comm);
    proc_info->cpu_time = (unsigned long long)(utime + stime);  // Total CPU time (user + kernel)

    return 0;
}

void sort_processes_by_cpu_time(process_info processes[], int process_count) {
    for (int i = 1; i < process_count; i++) {
        process_info key = processes[i];
        int j = i - 1;

        // Move elements of processes[0..i-1] that are less than key.cpu_time
        // to one position ahead of their current position
        while (j >= 0 && processes[j].cpu_time < key.cpu_time) {
            processes[j + 1] = processes[j];
            j--;
        }
        processes[j + 1] = key;
    }
}

void fetch_cpu_processes(process_info *top_procs, int top_n) {
    struct dirent *entry;
    DIR *proc_dir = opendir("/proc");

    process_info processes[1024];
    int process_count = 0;

    if (!proc_dir) {
        perror("Failed to open /proc directory");
        return;
    }

    // Iterate through the /proc directory to find process directories
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {  // Check if the directory name is a PID (numeric)
            
            if (isdigit(entry->d_name[0])) {
                char pid_dir[512];
                snprintf(pid_dir, sizeof(pid_dir), "%s/%s", "/proc", entry->d_name);

                // Parse the /proc/[pid]/stat file
                if (parse_stat_file(pid_dir, &processes[process_count]) == 0) {
                    process_count++;
                }
            }
        }
    }
    closedir(proc_dir);

    sort_processes_by_cpu_time(processes, process_count);

    // Copy the top `top_n` processes to the output array
    for (int i = 0; i < top_n && i < process_count; i++) {
        top_procs[i] = processes[i];
    }
}

void initialize_sockaddr_in(struct sockaddr_in *address) {
    // Check if the provided pointer is not NULL
    if (address == NULL) {
        fprintf(stderr, "Error: address pointer is NULL.\n");
        return;
    }
    
    address->sin_family = AF_INET;                // IPv4
    address->sin_addr.s_addr = INADDR_ANY;       // Accept connections from any address
    address->sin_port = htons(PORT);               // Set the port number (in network byte order)
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    client_info clients[MAX_CLIENTS];  // Array to store client info
    int client_count = 0;               // Count of current clients

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    initialize_sockaddr_in(&address);
    // address.sin_family = AF_INET;
    // address.sin_addr.s_addr = INADDR_ANY;
    // address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d\n", PORT);

    // Accept clients and handle them in a single-threaded manner
    while (1) {
        char buffer[BUFFER_SIZE] = {0};

        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept");
            exit(EXIT_FAILURE);
        }

        // Store client information
        if (client_count < MAX_CLIENTS) {
            clients[client_count].client_id = client_count + 1;  // Simple client ID assignment
            clients[client_count].socket = new_socket;
            printf("Server: New client connected (ID: %d)\n", clients[client_count].client_id);
            client_count++;
        } else {
            printf("Server: Max clients reached. Rejecting new connection.\n");
            close(new_socket);
            continue;
        }

        // Wait for the client's request
        int valread = read(new_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            printf("Server: Received request from client ID %d: %s\n", clients[client_count - 1].client_id, buffer);
            // Prepare to send the top CPU-consuming processes to the client
            process_info top_procs[2];
            fetch_cpu_processes(top_procs, 2);

            // Send the information to the client
            snprintf(buffer, BUFFER_SIZE,
                     "Top 2 CPU-consuming processes:\n1. Name: %s, PID: %d, CPU Time: %llu\n2. Name: %s, PID: %d, CPU Time: %llu\n",
                     top_procs[0].name, top_procs[0].pid, top_procs[0].cpu_time,
                     top_procs[1].name, top_procs[1].pid, top_procs[1].cpu_time);

            send(new_socket, buffer, strlen(buffer), 0);
            printf("Server: Sent CPU process info to client ID %d\n", clients[client_count - 1].client_id);
        }

        close(new_socket);
        printf("Server: Client ID %d disconnected\n", clients[client_count - 1].client_id);
    }

    close(server_fd);
    return 0;
}
