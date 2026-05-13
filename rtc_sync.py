import socket
import time
import requests
import subprocess
import logging

### PIN OUTS
#SDA - GPIO2
#SCL - GPIO3
#VCC - 3.3v


# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')

def is_connected():
    """Check if the Pi has internet access by reaching out to Google's DNS."""
    try:
        socket.create_connection(("8.8.8.8", 53), timeout=3)
        return True
    except OSError:
        pass
    return False

def sync_from_internet():
    """Fetch timezone via IP and time from Google's HTTP headers, then update system and RTC."""
    try:
        logging.info("Internet connected. Fetching timezone and time...")
        
        # 1. Get Timezone from ip-api.com (Free, no auth required)
        tz_response = requests.get("http://ip-api.com/json/", timeout=10)
        tz_response.raise_for_status()
        timezone = tz_response.json().get('timezone')
        
        if not timezone:
            raise ValueError("Timezone data missing from API response.")
            
        # Set the system timezone
        subprocess.run(["sudo", "timedatectl", "set-timezone", timezone], check=True)
        logging.info(f"Timezone set to {timezone}")
        
        # 2. Get Time from Google's server headers
        # Google's servers return a 'Date' header in standard GMT (e.g., 'Wed, 13 May 2026 09:15:52 GMT')
        time_response = requests.head("https://google.com", timeout=10)
        google_date_str = time_response.headers.get('Date')
        
        if not google_date_str:
            raise ValueError("Date header missing from Google response.")
            
        # Set the system time (the 'date' command automatically parses the GMT string correctly)
        subprocess.run(["sudo", "date", "-s", google_date_str], check=True)
        logging.info(f"System time set to {google_date_str}")
        
        # 3. Write the accurate internet time back to the RTC module
        logging.info("Updating RTC module with new internet time...")
        subprocess.run(["sudo", "hwclock", "-w"], check=True)
        
        logging.info("Successfully synced time and timezone from internet.")
        return True
        
    except Exception as e:
        logging.error(f"Failed to sync from internet: {e}")
        return False

def sync_from_rtc():
    """Read time from the RTC module and apply it to the system."""
    try:
        logging.info("Syncing system time from RTC module...")
        subprocess.run(["sudo", "hwclock", "-s"], check=True)
        logging.info("System time successfully updated from RTC.")
    except Exception as e:
        logging.error(f"Failed to sync from RTC. Is the module wired and configured correctly? Error: {e}")

def get_uptime():
    """Get system uptime in seconds directly from the kernel."""
    with open('/proc/uptime', 'r') as f:
        uptime_seconds = float(f.readline().split()[0])
    return uptime_seconds

def main():
    # Wait a few seconds to give network interfaces a chance to initialize after boot
    time.sleep(5) 
    
    if is_connected():
        success = sync_from_internet()
        if not success:
            # Fallback to RTC if the internet is up but our API/HTTP requests fail
            sync_from_rtc()
    else:
        logging.info("No internet connection detected.")
        uptime = get_uptime()
        
        # Calculate how much time is left to reach 30 seconds of uptime
        wait_time = 30.0 - uptime
        
        if wait_time > 0:
            logging.info(f"System up for {uptime:.1f}s. Waiting {wait_time:.1f}s to reach the 30-second mark...")
            time.sleep(wait_time)
        else:
            logging.info("System has already been up for more than 30 seconds.")
            
        sync_from_rtc()

if __name__ == "__main__":
    main()
