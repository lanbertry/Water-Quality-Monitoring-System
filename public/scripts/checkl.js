firebase.auth().onAuthStateChanged(function (user) {
  if (user) {
    // User is already logged in, redirect to dashboard
    console.log("User is logged in:", user.email);
    window.location.href = "dashboard.html";
  } else {
    console.log("No user logged in. Stay on login page.");
  }
});
