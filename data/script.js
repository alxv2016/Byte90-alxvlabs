// DOM Elements
const elements = {
  password: document.getElementById("password"),
  toggleButton: document.querySelector(".password-toggle"),
  showIcon: document.querySelector(".password-toggle svg:first-child"),
  hideIcon: document.querySelector(".password-toggle svg:last-child"),
  disconnectBtn: document.getElementById("disconnectBtn"),
  connectBtn: document.getElementById("connectBtn"),
  scanBtn: document.getElementById("scanBtn"),
  statusNotification: document.getElementById("wifiStatus"),
  updateStatusNotification: document.getElementById("updateStatus"),
  networkSelect: document.getElementById("networks"),
  passwordFormControl: document.getElementById("passwordFormControl"),
  updateForm: document.getElementById("updateForm"),
  progressContainer: document.getElementById("progressContainer"),
  progressBar: document.getElementById("uploadProgress"),
  progressText: document.getElementById("progressText"),
  uploadBtn: document.getElementById("uploadBtn"),
};

// State
const state = {
  isPasswordVisible: false,
};

// UI Helper Functions
const ui = {
  showStatus(el, message, type) {
    el.textContent = message;
    el.classList.remove("status-warning", "status-success", "status-error");

    const typeMap = {
      ERROR: "status-danger",
      SUCCESS: "status-success",
      WARNING: "status-warning",
    };

    if (typeMap[type]) {
      el.classList.add(typeMap[type]);
    }

    el.style.display = "flex";
  },

  hideStatus(el) {
    el.textContent = "";
    el.classList.remove("status-warning", "status-success", "status-error");
    el.removeAttribute("style");
  },

  showDisconnectState() {
    const {
      disconnectBtn,
      connectBtn,
      scanBtn,
      passwordFormControl,
      networkSelect,
    } = elements;
    disconnectBtn.style.display = "none";
    connectBtn.removeAttribute("style");
    scanBtn.removeAttribute("style");
    passwordFormControl.removeAttribute("style");
    networkSelect.selectedIndex = 0;
    networkSelect.removeAttribute("disabled");
    uploadBtn.setAttribute("disabled", "true");
    uploadBtn.classList.remove("btn-primary");
    uploadBtn.classList.add("btn-disabled");
  },

  showConnectState() {
    const {
      disconnectBtn,
      connectBtn,
      scanBtn,
      passwordFormControl,
      networkSelect,
    } = elements;
    disconnectBtn.style.display = "inline-flex";
    connectBtn.style.display = "none";
    scanBtn.style.display = "none";
    passwordFormControl.style.display = "none";
    networkSelect.setAttribute("disabled", "true");
    uploadBtn.removeAttribute("disabled");
    uploadBtn.classList.remove("btn-disabled");
    uploadBtn.classList.add("btn-primary");
  },

  togglePasswordVisibility() {
    const { password, showIcon, hideIcon } = elements;
    state.isPasswordVisible = !state.isPasswordVisible;
    password.type = state.isPasswordVisible ? "text" : "password";
    showIcon.style.display = state.isPasswordVisible ? "none" : "block";
    hideIcon.style.display = state.isPasswordVisible ? "block" : "none";
  },

  initProgress() {
    elements.progressContainer.style.display = "block";
    elements.progressBar.value = 0;
    elements.progressText.textContent = "";
  },

  updateProgress(percent) {
    elements.progressBar.value = percent;
    elements.progressText.textContent = `Uploading: ${Math.round(percent)}%`;
  },

  completeProgress() {
    elements.progressBar.value = 100;
  },

  resetProgress() {
    elements.progressBar.value = 0;
    elements.progressContainer.removeAttribute("style");
    elements.progressText.textContent = "";
  },

  updateNetworkSelect(networks) {
    const select = elements.networkSelect;
    select.innerHTML = "";

    networks.forEach((network) => {
      const option = document.createElement("option");
      option.value = network.ssid;
      // Add a fallback in case signal_strength is not available
      const signalStr = network.signal_strength || "Signal strength unavailable";
      option.textContent = `${network.ssid} (${signalStr})`;
      select.appendChild(option);
    });
  },
};

// Network Operations
const network = {
  async checkConnectionStatus() {
    ui.hideStatus(elements.statusNotification);
    try {
      const response = await fetchWithTimeout("/status", {}, 60000);
      if (!response.ok) {
        throw new Error(
          `Connection status check failed with status: ${response.status}`
        );
      }
      const data = await response.json();
      console.log(data);
      if (data.success) {
        ui.showStatus(elements.statusNotification, data.message, "SUCCESS");
      }
      return data;
    } catch (error) {
      console.error("Connection status check error:", error);
      ui.showStatus(
        elements.statusNotification,
        "Failed to check connection status",
        "ERROR"
      );
      return { success: false };
    }
  },

  async scanNetworks() {
    ui.hideStatus(elements.statusNotification);
    ui.showStatus(
      elements.statusNotification,
      "Scanning for networks",
      "WARNING"
    );
    try {
      const response = await fetchWithTimeout("/scan", {}, 60000);
      if (!response.ok) {
        throw new Error(`Network scan failed with status: ${response.status}`);
      }

      const data = await response.json();
      if (!data.success) {
        ui.showStatus(elements.statusNotification, data.message, "ERROR");
        return data.networks;
      }

      const sortedNetworks = data.networks.sort((a, b) => b.rssi - a.rssi);
      ui.showStatus(elements.statusNotification, data.message, "SUCCESS");
      return sortedNetworks;
    } catch (error) {
      console.error("Network scan error:", error);
      ui.showStatus(
        elements.statusNotification,
        "Failed to scan networks, please try again",
        "ERROR"
      );
      return [];
    }
  },

  async connect() {
    ui.hideStatus(elements.statusNotification);
    const ssid = elements.networkSelect.value.trim();
    const password = elements.password.value.trim();

    if (!ssid) {
      ui.showStatus(
        elements.statusNotification,
        "Please select a network",
        "ERROR"
      );
      return { success: false, message: "No network selected" };
    }

    if (!password) {
      ui.showStatus(
        elements.statusNotification,
        "Please enter Wi-Fi password",
        "ERROR"
      );
      return { success: false, message: "No password provided" };
    }

    ui.showStatus(
      elements.statusNotification,
      `Attempting to connect to ${ssid}...`,
      "WARNING"
    );

    try {
      return await connectionWithRetry(async () => {
        const response = await fetch("/connect", {
          method: "POST",
          headers: {
            "Content-Type": "application/x-www-form-urlencoded",
          },
          body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(
            password
          )}`,
        });
  
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
  
        const data = await response.json();
        ui.showStatus(
          elements.statusNotification,
          data.message,
          data.success ? "SUCCESS" : "ERROR"
        );
        if (data.success) {
          const response = await init();
          return response;
        } else {
          return data;
        }
      });
    } catch (error) {
      console.error("Connection error:", error);
      ui.showStatus(
        elements.statusNotification,
        "Failed to connect to network",
        "ERROR"
      );
      return { success: false, message: "Connection failed" };
    }
  },
  async disconnect() {
    ui.hideStatus(elements.statusNotification);
    try {
      const response = await fetchWithTimeout("/disconnect", { method: "POST" }, 60000);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      const data = await response.json();
      ui.showStatus(
        elements.statusNotification,
        data.message,
        data.success ? "SUCCESS" : "ERROR"
      );
      if (data.success) {
        const response = await init();
        return response;
      } else {
        return data;
      }
    } catch (error) {
      console.error("Connection error:", error);
      ui.showStatus(
        elements.statusNotification,
        "Failed to disconnect from Wi-Fi network.",
        "ERROR"
      );
      return { success: false, message: "Connection timed out." };
    }
  },

  async restart() {
    ui.hideStatus(elements.statusNotification);
    try {
      const response = await fetchWithTimeout("/restart", { method: "POST" }, 60000);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      const data = await response.json();
      ui.showStatus(
        elements.statusNotification,
        data.message,
        data.success ? "SUCCESS" : "ERROR"
      );
      return data;
    } catch (error) {
      console.error("Connection error:", error);
      ui.showStatus(
        elements.statusNotification,
        "Failed to disconnect from Wi-Fi network.",
        "ERROR"
      );
      return { success: false, message: "Connection timed out." };
    }
  },

  async pollUpdateStatus() {
    try {
      const response = await fetchWithTimeout("/update/status", {}, 60000);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      const data = await response.json();
      return data;
    } catch (error) {
      console.error("Failed to get update status:", error);
      return null;
    }
  },
};

async function connectionWithRetry(operation, maxRetries = 3) {
  let attempts = 0;
  while (attempts < maxRetries) {
    try {
      return await operation();
    } catch (error) {
      attempts++;
      if (attempts === maxRetries) throw error;
      
      // Wait longer between each retry
      await new Promise(resolve => setTimeout(resolve, 1000 * attempts));
      ui.showStatus(
        elements.statusNotification,
        `Retry attempt ${attempts}...`,
        "WARNING"
      );
    }
  }
}

const fetchWithTimeout = (url, options, timeout = 30000) => {
  return Promise.race([
    fetch(url, options),
    new Promise((_, reject) => 
      setTimeout(() => reject(new Error('Request timeout')), timeout)
    )
  ]);
};

function validateFirmwareFile(file) {
  // Check file type
  if (!file.name.endsWith('.bin')) {
    return { valid: false, message: "Invalid firmware file format. Expected .bin file." };
  }
  
  // Check file size (adjust max size as needed)
  if (file.size > 10 * 1024 * 1024) { // 10MB limit
    return { valid: false, message: "Firmware file too large. Maximum size is 10MB." };
  }
  
  return { valid: true };
}

// Firmware update handler
function handleFirmwareUpdate(e) {
  e.preventDefault();
  ui.hideStatus(elements.updateStatusNotification);
  const form = e.target;
  const formData = new FormData(form);
  const firmwareFile = formData.get("firmwareFile");

  if (firmwareFile) {
    const validation = validateFirmwareFile(firmwareFile);
    if (!validation.valid) {
      ui.showStatus(elements.updateStatusNotification, validation.message, "ERROR");
      return;
    }
  } else {
    ui.showStatus(elements.updateStatusNotification, "No firmware file selected", "ERROR");
    return;
  }

  ui.initProgress();

  const xhr = new XMLHttpRequest();
  xhr.open("POST", form.action, true);

  const handleError = (message) => {
    ui.resetProgress();
    ui.showStatus(elements.updateStatusNotification, message, "ERROR");
    if (statusInterval) {
      clearInterval(statusInterval);
    }
  };

  // Start polling when upload begins
  let statusInterval = null;
  xhr.upload.onloadstart = () => {
    statusInterval = setInterval(async () => {
      const status = await network.pollUpdateStatus();
      if (status) {
        ui.updateProgress(status.progress);
        ui.showStatus(
          elements.updateStatusNotification,
          status.message,
          status.state === "ERROR"
            ? "ERROR"
            : status.state === "SUCCESS"
            ? "SUCCESS"
            : "WARNING"
        );

        // Clear interval if we're done or there's an error
        if (status.completed || status.state === "ERROR") {
          clearInterval(statusInterval);
          if (status.state === "SUCCESS") {
            setTimeout(() => window.location.reload(), 2000);
          }
        }
      }
    }, 1000); // Poll every second
  };

  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      const percentComplete = (e.loaded / e.total) * 100;
      ui.updateProgress(percentComplete);
    }
  };

  xhr.onload = () => {
    // Clear polling interval when upload is complete
    if (statusInterval) {
      clearInterval(statusInterval);
    }
    try {
      const response = JSON.parse(xhr.responseText);

      switch (response.state) {
        case "SUCCESS":
          ui.completeProgress();
          ui.showStatus(
            elements.updateStatusNotification,
            response.message,
            "SUCCESS"
          );
          setTimeout(() => window.location.reload(), 2000);
          break;
        case "ERROR":
          handleError(response.message);
          break;
        default:
          ui.updateProgress(response.progress);
      }
    } catch (e) {
      handleError("Error communicating with device, please try again.");
    }
  };

  xhr.onerror = () => {
    if (statusInterval) {
      clearInterval(statusInterval);
    }
    handleError(
      "Network timed out, make sure you are connected to your Wi-Fi network."
    );
  };

  xhr.send(formData);
}

// Event Listeners
function initializeEventListeners() {
  // Password visibility toggle
  elements.toggleButton.addEventListener("click", ui.togglePasswordVisibility);
  elements.toggleButton.addEventListener("keydown", (event) => {
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      ui.togglePasswordVisibility();
    }
  });
  // Network operations
  elements.scanBtn.addEventListener("click", network.scanNetworks);
  elements.connectBtn.addEventListener("click", network.connect);
  elements.disconnectBtn.addEventListener("click", network.disconnect);

  elements.updateForm.onsubmit = handleFirmwareUpdate;
}

window.addEventListener('error', (event) => {
  console.error('Global error:', event.error);
  ui.showStatus(
    elements.statusNotification,
    "An unexpected error occurred. Please refresh the page.",
    "ERROR"
  );
});

// Initialize Application
async function init() {
  elements.hideIcon.style.display = "none";
  initializeEventListeners();
  // First, check our current connection status
  const status = await network.checkConnectionStatus();

  if (status.success) {
    // We're connected - update UI to show current network
    ui.showConnectState();
    const currentNetwork = [
      {
        ssid: status.ssid,
        rssi: status.rssi,
        signal_strength: status.signal_strength || "Not available",
      },
    ];
    ui.updateNetworkSelect(currentNetwork);
  } else {
    // We're disconnected - scan and show available networks
    ui.showDisconnectState();
    const networks = await network.scanNetworks();
    ui.updateNetworkSelect(networks);
    elements.password.focus();
  }
}

init();