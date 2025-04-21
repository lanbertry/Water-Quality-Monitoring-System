// Create the charts when the web page loads
window.addEventListener("load", onload);

function onload(event) {
  chartT = createTemperatureChart();
  chartP = createPhChart();
  chartTu = createTurbidityChart();
}

// Create Temperature Chart with range selector
function createTemperatureChart() {
  var chart = new Highcharts.Chart({
    chart: {
      renderTo: "chart-temperature",
      type: "spline",
    },
    series: [
      {
        name: "Water Temperature",
      },
    ],
    time: {
      useUTC: false,
    },
    title: {
      text: "",
    },
    exporting: {
      tableCaption: "Water Temperature", // Custom title for the data table
      printMaxWidth: 1000, // Allow more width for printing
      chartOptions: {
        exporting: {
          showTable: true, // Ensure table shows in print
        },
      },
    },
    rangeSelector: {
      buttons: [
        {
          type: "hour",
          count: 1,
          text: "1h",
        },
        {
          type: "day",
          count: 1,
          text: "1d",
        },
        {
          type: "week",
          count: 1,
          text: "1w",
        },
        {
          type: "month",
          count: 1,
          text: "1m",
        },
        {
          type: "all",
          text: "All",
        },
      ],
      selected: 1,
      inputEnabled: false,
    },
    plotOptions: {
      line: {
        animation: false,
        dataLabels: {
          enabled: true,
        },
      },
      series: {
        color: "#059e05",
      },
    },
    xAxis: {
      type: "datetime",
      dateTimeLabelFormats: {
        second: "%H:%M:%S",
        minute: "%H:%M",
        hour: "%H:%M",
        day: "%e. %b",
        week: "%e. %b",
        month: "%b '%y",
        year: "%Y",
      },
    },
    yAxis: {
      title: {
        text: "Temperature Celsius Degrees",
      },
    },
    credits: {
      enabled: false,
    },
  });
  return chart;
}

// Create Ph Chart with range selector
function createPhChart() {
  var chart = new Highcharts.Chart({
    chart: {
      renderTo: "chart-ph",
      type: "spline",
    },
    series: [
      {
        name: "pH Sensor",
      },
    ],
    time: {
      useUTC: false,
    },
    title: {
      text: undefined,
    },
    exporting: {
      tableCaption: "pH Level", // Custom title for the data table
      printMaxWidth: 1000, // Allow more width for printing
      chartOptions: {
        exporting: {
          showTable: true, // Ensure table shows in print
        },
      },
    },
    rangeSelector: {
      buttons: [
        {
          type: "hour",
          count: 1,
          text: "1h",
        },
        {
          type: "day",
          count: 1,
          text: "1d",
        },
        {
          type: "week",
          count: 1,
          text: "1w",
        },
        {
          type: "month",
          count: 1,
          text: "1m",
        },
        {
          type: "all",
          text: "All",
        },
      ],
      selected: 1,
      inputEnabled: false,
    },
    plotOptions: {
      line: {
        animation: false,
        dataLabels: {
          enabled: true,
        },
      },
      series: {
        color: "#e60505",
      },
    },
    xAxis: {
      type: "datetime",
      dateTimeLabelFormats: {
        second: "%H:%M:%S",
        minute: "%H:%M",
        hour: "%H:%M",
        day: "%e. %b",
        week: "%e. %b",
        month: "%b '%y",
        year: "%Y",
      },
    },
    yAxis: {
      title: {
        text: "pH Level",
      },
    },
    credits: {
      enabled: false,
    },
  });
  return chart;
}

// Create Turbidity Chart with range selector
function createTurbidityChart() {
  var chart = new Highcharts.Chart({
    chart: {
      renderTo: "chart-turbidity",
      type: "spline",
    },
    series: [
      {
        name: "Turbidity Sensor",
      },
    ],
    time: {
      useUTC: false,
    },
    title: {
      text: undefined,
    },
    exporting: {
      tableCaption: "Turbidity Sensor", // Custom title for the data table
      printMaxWidth: 1000, // Allow more width for printing
      chartOptions: {
        exporting: {
          showTable: true, // Ensure table shows in print
        },
      },
    },
    rangeSelector: {
      buttons: [
        {
          type: "hour",
          count: 1,
          text: "1h",
        },
        {
          type: "day",
          count: 1,
          text: "1d",
        },
        {
          type: "week",
          count: 1,
          text: "1w",
        },
        {
          type: "month",
          count: 1,
          text: "1m",
        },
        {
          type: "all",
          text: "All",
        },
      ],
      selected: 1,
      inputEnabled: false,
    },
    plotOptions: {
      line: {
        animation: false,
        dataLabels: {
          enabled: true,
        },
      },
      series: {
        color: "#e08b0c",
      },
    },
    xAxis: {
      type: "datetime",
      dateTimeLabelFormats: {
        second: "%H:%M:%S",
        minute: "%H:%M",
        hour: "%H:%M",
        day: "%e. %b",
        week: "%e. %b",
        month: "%b '%y",
        year: "%Y",
      },
    },
    yAxis: {
      title: {
        text: "Turbidity Level",
      },
    },
    credits: {
      enabled: false,
    },
  });
  return chart;
}
