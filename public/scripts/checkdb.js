firebase.auth().onAuthStateChanged(function (user) {
  if (user) {
    /*     console.log("user logged in"); */
  } else {
    window.location.href = "index.html";
  }
});
