[README.md](https://github.com/user-attachments/files/25847714/README.md)
# CSCE 612 — Computer Networks

Projects completed for **CSCE 612: Networks & Distributed Processing** at Texas A&M University, Spring 2026.

All projects are implemented in **C++** and focus on building core internet infrastructure from the ground up — from application-layer crawling and DNS resolution to low-level socket programming.

---

## HW1 — Multi-Threaded Web Crawler

A high-performance, multi-threaded web crawler built using **Winsock2** and **OpenSSL**.

**Highlights:**
- Crawls up to **~1 million unique URLs** using approximately **3,500 concurrent threads**
- Performs HTTP/HTTPS requests with proper header parsing and redirect handling
- Implements DNS resolution, robots.txt compliance checking, and URL normalization
- Tracks crawl statistics (pages/sec, bytes downloaded, unique hosts, etc.) with a dedicated stats thread
- Thread-safe data structures for managing the URL frontier and visited sets

**Key Concepts:** Socket programming, TLS/SSL, multi-threading, synchronization, HTTP protocol

---

## HW2 — DNS Client

A fully functional **DNS resolver** built from scratch in C++.

**Highlights:**
- Constructs and sends raw DNS query packets (type A, CNAME, PTR, etc.) over UDP
- Parses DNS response packets including header, question, answer, authority, and additional sections
- Handles compressed domain names (pointer-based label compression per RFC 1035)
- Supports iterative resolution starting from root DNS servers
- Implements retransmission with configurable timeouts

**Key Concepts:** DNS protocol (RFC 1035), UDP sockets, binary packet construction/parsing, network byte order

---

## Build & Run

Each homework is self-contained in its own directory. Refer to the individual project folders for specific build instructions and dependencies.

```
CSCE612/
├── HW1/            # Multi-Threaded Web Crawler
└── HW2-CSCE612/    # DNS Client
```

> **Note:** Projects were developed on Windows using Visual Studio with Winsock2. Compilation on other platforms may require adjustments.
