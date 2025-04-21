// Convert epochtime to JavaScript Date object
function epochToJsDate(epochTime) {
  return new Date(epochTime * 1000);
}

// Convert time to human-readable format YYYY/MM/DD HH:MM:SS
function epochToDateTime(epochTime) {
  var epochDate = new Date(epochToJsDate(epochTime));
  var year = epochDate.getFullYear();
  var month = ("00" + (epochDate.getMonth() + 1)).slice(-2);
  var day = ("00" + epochDate.getDate()).slice(-2);
  var hours = epochDate.getHours();
  var minutes = ("00" + epochDate.getMinutes()).slice(-2);
  var seconds = ("00" + epochDate.getSeconds()).slice(-2);
  var ampm = hours >= 12 ? "PM" : "AM";
  hours = hours % 12;
  hours = hours ? hours : 12;
  hours = ("00" + hours).slice(-2);
  return `${year}/${month}/${day} ${hours}:${minutes}:${seconds} ${ampm}`;
}

let chartT, chartP, chartTu;
let currentData = [];
let currentRange = "24h";

// Function to calculate averages for time periods
function calculateAverages(data, interval) {
  if (data.length === 0) return [];

  const averagedData = [];
  let currentInterval = null;
  let intervalData = [];

  // Sort data by timestamp (oldest first)
  const sortedData = [...data].sort((a, b) => a.timestamp - b.timestamp);

  sortedData.forEach((item) => {
    const date = epochToJsDate(item.timestamp);
    let intervalKey;

    if (interval === "hourly") {
      intervalKey = `${date.getFullYear()}-${date.getMonth()}-${date.getDate()}-${date.getHours()}`;
    } else if (interval === "daily") {
      intervalKey = `${date.getFullYear()}-${date.getMonth()}-${date.getDate()}`;
    } else if (interval === "weekly") {
      const weekNumber = Math.floor(date.getDate() / 7);
      intervalKey = `${date.getFullYear()}-${date.getMonth()}-${weekNumber}`;
    }

    if (intervalKey !== currentInterval) {
      if (intervalData.length > 0) {
        const count = intervalData.length;
        const avgTemp =
          intervalData.reduce((sum, d) => sum + parseFloat(d.temperature), 0) /
          count;
        const avgPh =
          intervalData.reduce((sum, d) => sum + parseFloat(d.ph_level), 0) /
          count;
        const avgTurbidity =
          intervalData.reduce((sum, d) => sum + parseFloat(d.turbidity), 0) /
          count;
        const avgTimestamp = intervalData[Math.floor(count / 2)].timestamp;

        averagedData.push({
          timestamp: avgTimestamp,
          temperature: parseFloat(avgTemp.toFixed(2)),
          ph_level: parseFloat(avgPh.toFixed(2)),
          turbidity: parseFloat(avgTurbidity.toFixed(2)),
        });
      }

      currentInterval = intervalKey;
      intervalData = [item];
    } else {
      intervalData.push(item);
    }
  });

  // Add the last interval
  if (intervalData.length > 0) {
    const count = intervalData.length;
    const avgTemp =
      intervalData.reduce((sum, d) => sum + parseFloat(d.temperature), 0) /
      count;
    const avgPh =
      intervalData.reduce((sum, d) => sum + parseFloat(d.ph_level), 0) / count;
    const avgTurbidity =
      intervalData.reduce((sum, d) => sum + parseFloat(d.turbidity), 0) / count;
    const avgTimestamp = intervalData[Math.floor(count / 2)].timestamp;

    averagedData.push({
      timestamp: avgTimestamp,
      temperature: parseFloat(avgTemp.toFixed(2)),
      ph_level: parseFloat(avgPh.toFixed(2)),
      turbidity: parseFloat(avgTurbidity.toFixed(2)),
    });
  }

  return averagedData;
}

// Function to process data based on current range
function processDataForRange(data, range) {
  const now = Date.now() / 1000;
  let filteredData = data;

  // Filter data by time range
  if (range === "1h") {
    filteredData = data.filter((item) => item.timestamp > now - 3600);
    // Return raw data in chronological order for hourly view
    return filteredData.sort((a, b) => a.timestamp - b.timestamp);
  } else if (range === "24h") {
    filteredData = data.filter((item) => item.timestamp > now - 86400);
    return calculateAverages(filteredData, "hourly");
  } else if (range === "7d") {
    filteredData = data.filter((item) => item.timestamp > now - 604800);
    return calculateAverages(filteredData, "daily");
  } else if (range === "30d") {
    filteredData = data.filter((item) => item.timestamp > now - 2592000);
    return calculateAverages(filteredData, "weekly");
  }

  return filteredData.sort((a, b) => a.timestamp - b.timestamp);
}

// Function to update all charts with processed data
function updateCharts(processedData) {
  try {
    // Convert data to Highcharts format
    const tempData = processedData.map((item) => [
      epochToJsDate(item.timestamp).getTime(),
      parseFloat(item.temperature),
    ]);

    const phData = processedData.map((item) => [
      epochToJsDate(item.timestamp).getTime(),
      parseFloat(item.ph_level),
    ]);

    const turbData = processedData.map((item) => [
      epochToJsDate(item.timestamp).getTime(),
      parseFloat(item.turbidity),
    ]);

    // Update chart data with animation but without excessive redrawing
    if (chartT && chartT.series && chartT.series[0]) {
      chartT.series[0].setData(tempData, true, false, true);
    }
    if (chartP && chartP.series && chartP.series[0]) {
      chartP.series[0].setData(phData, true, false, true);
    }
    if (chartTu && chartTu.series && chartTu.series[0]) {
      chartTu.series[0].setData(turbData, true, false, true);
    }
  } catch (error) {
    console.error("Error updating charts:", error);
  }
}

// Function to plot a single new data point
function plotNewPoint(data) {
  // Add new data point
  currentData.push(data);

  // Clean up old data (older than 30 days to prevent memory issues)
  const now = Date.now() / 1000;
  currentData = currentData.filter((item) => item.timestamp > now - 2592000);

  // Process data based on current range
  const processedData = processDataForRange(currentData, currentRange);

  // For hourly view, ensure we only keep the last hour's data
  if (currentRange === "1h") {
    const oneHourAgo = now - 3600;
    const hourlyData = processedData.filter(
      (item) => item.timestamp > oneHourAgo
    );
    updateCharts(hourlyData);
  } else {
    updateCharts(processedData);
  }
}

// DOM elements
const contentElement = document.querySelector("#content-sign-in");
const userDetailsElement = document.querySelector("#user-details");
const authBarElement = document.querySelector("#authentication-bar");
const deleteButtonElement = document.getElementById("delete-button");
const deleteModalElement = document.getElementById("delete-modal");
const deleteDataFormElement = document.querySelector("#delete-data-form");
const viewDataButtonElement = document.getElementById("view-data-button");
const hideDataButtonElement = document.getElementById("hide-data-button");
const tableContainerElement = document.querySelector("#table-container");
const chartsRangeInputElement = document.getElementById("charts-range");
const loadDataButtonElement = document.getElementById("load-data");
const cardsCheckboxElement = document.querySelector(
  "input[name=cards-checkbox]"
);
const chartsCheckboxElement = document.querySelector(
  "input[name=charts-checkbox]"
);
const cardsReadingsElement = document.querySelector("#cards-div");
const chartsDivElement = document.querySelector("#charts-div");
const tempElement = document.getElementById("temp");
const phElement = document.getElementById("ph");
const turbElement = document.getElementById("turb");
const updateElement = document.getElementById("lastUpdate");

// MANAGE LOGIN/LOGOUT UI
const setupUI = (user) => {
  if (user) {
    // Toggle UI elements
    contentElement.style.display = "block";
    authBarElement.style.display = "block";
    userDetailsElement.style.display = "block";
    userDetailsElement.innerHTML = user.email;

    // Get user UID
    const uid = user.uid;
    const dbPath = `UsersData/${uid}/readings`;
    const dbRef = firebase.database().ref(dbPath);

    // Initialize charts
    chartT = createTemperatureChart();
    chartP = createPhChart();
    chartTu = createTurbidityChart();

    // Load initial data
    dbRef
      .once("value")
      .then((snapshot) => {
        if (snapshot.exists()) {
          currentData = [];
          snapshot.forEach((child) => {
            const data = child.toJSON();
            // Ensure all values are numbers
            data.temperature = parseFloat(data.temperature);
            data.ph_level = parseFloat(data.ph_level);
            data.turbidity = parseFloat(data.turbidity);
            currentData.push(data);
          });

          const processedData = processDataForRange(currentData, currentRange);
          console.log("Initial data loaded:", processedData);
          updateCharts(processedData);
        } else {
          console.log("No data available");
        }
      })
      .catch((error) => {
        console.error("Error loading initial data:", error);
      });

    // Real-time updates
    dbRef.limitToLast(1).on("child_added", (snapshot) => {
      const newData = snapshot.toJSON();
      // Ensure all values are numbers
      newData.temperature = parseFloat(newData.temperature);
      newData.ph_level = parseFloat(newData.ph_level);
      newData.turbidity = parseFloat(newData.turbidity);

      // Update cards with latest reading
      tempElement.innerHTML = newData.temperature.toFixed(2);
      phElement.innerHTML = newData.ph_level.toFixed(2);
      turbElement.innerHTML = newData.turbidity.toFixed(2);
      updateElement.innerHTML = epochToDateTime(newData.timestamp);

      // Update charts
      plotNewPoint(newData);
    });

    // Update chart range when dropdown changes
    chartsRangeInputElement.addEventListener("change", () => {
      currentRange = chartsRangeInputElement.value;
      const processedData = processDataForRange(currentData, currentRange);
      console.log(
        "Range changed to",
        currentRange,
        "with data:",
        processedData
      );
      updateCharts(processedData);
    });

    // Checkbox handlers
    cardsCheckboxElement.addEventListener("change", () => {
      cardsReadingsElement.style.display = cardsCheckboxElement.checked
        ? "block"
        : "none";
    });

    chartsCheckboxElement.addEventListener("change", () => {
      chartsDivElement.style.display = chartsCheckboxElement.checked
        ? "block"
        : "none";
    });

    // Delete data handler
    deleteButtonElement.addEventListener("click", (e) => {
      e.preventDefault();
      deleteModalElement.style.display = "block";
    });

    deleteDataFormElement.addEventListener("submit", (e) => {
      e.preventDefault();
      dbRef
        .remove()
        .then(() => {
          currentData = [];
          updateCharts([]);
          deleteModalElement.style.display = "none";
        })
        .catch((error) => {
          console.error("Error deleting data:", error);
        });
    });

    // Table functionality
    let lastReadingTimestamp;

    function createTable() {
      dbRef
        .orderByKey()
        .limitToLast(100)
        .once("value", function (snapshot) {
          if (snapshot.exists()) {
            var firstRun = true;
            snapshot.forEach((childSnapshot) => {
              var jsonData = childSnapshot.toJSON();
              var temperature = jsonData.temperature;
              var ph_level = jsonData.ph_level;
              var turbidity = jsonData.turbidity;
              var timestamp = jsonData.timestamp;

              var content = "<tr>";
              content += "<td>" + epochToDateTime(timestamp) + "</td>";
              content += "<td>" + temperature + "</td>";
              content += "<td>" + ph_level + "</td>";
              content += "<td>" + turbidity + "</td>";
              content += "</tr>";
              $("#tbody").prepend(content);

              if (firstRun) {
                lastReadingTimestamp = timestamp;
                firstRun = false;
              }
            });
          }
        });
    }

    function appendToTable() {
      const olderData = currentData
        .filter((item) => item.timestamp < lastReadingTimestamp)
        .sort((a, b) => b.timestamp - a.timestamp)
        .slice(0, 100); // Get next 100 older readings

      if (olderData.length === 0) {
        loadDataButtonElement.style.display = "none";
        return;
      }

      olderData.forEach((data) => {
        const row = `<tr>
          <td>${epochToDateTime(data.timestamp)}</td>
          <td>${data.temperature.toFixed(2)}</td>
          <td>${data.ph_level.toFixed(2)}</td>
          <td>${data.turbidity.toFixed(2)}</td>
        </tr>`;
        $("#tbody").append(row);
      });

      lastReadingTimestamp = olderData[olderData.length - 1].timestamp;
    }

    /*     function createTable() {
      $("#tbody").empty();
      const tableData = processDataForRange(currentData, "24h"); // Show raw data for table

      tableData.forEach((data) => {
        const row = `<tr>
          <td>${epochToDateTime(data.timestamp)}</td>
          <td>${data.temperature.toFixed(2)}</td>
          <td>${data.ph_level.toFixed(2)}</td>
          <td>${data.turbidity.toFixed(2)}</td>
        </tr>`;
        $("#tbody").append(row);
      });

      if (tableData.length > 0) {
        lastReadingTimestamp = tableData[tableData.length - 1].timestamp;
      }
    } */

    viewDataButtonElement.addEventListener("click", () => {
      tableContainerElement.style.display = "block";
      viewDataButtonElement.style.display = "none";
      hideDataButtonElement.style.display = "inline-block";
      loadDataButtonElement.style.display = "inline-block";
      createTable();
    });

    loadDataButtonElement.addEventListener("click", appendToTable);

    hideDataButtonElement.addEventListener("click", () => {
      tableContainerElement.style.display = "none";
      viewDataButtonElement.style.display = "inline-block";
      hideDataButtonElement.style.display = "none";
    });
  } else {
    // User logged out
    contentElement.style.display = "none";
    authBarElement.style.display = "none";
  }
};

// Initialize Firebase Auth
firebase.auth().onAuthStateChanged((user) => {
  setupUI(user);
});

// Close modal when clicking outside
window.addEventListener("click", function (event) {
  if (event.target == deleteModalElement) {
    deleteModalElement.style.display = "none";
  }
});
