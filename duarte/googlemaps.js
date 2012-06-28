		  var googleInit = false;
		  
		function googleInitialize() {
		  	if(googleInit == false){
		  	    googleInit = true;
			    var latlng = new google.maps.LatLng(38.736691,-9.302105);
			    var myOptions = {
			      zoom: 18,
			      center: latlng,
			      mapTypeId: google.maps.MapTypeId.HYBRID
			    };
			    var map = new google.maps.Map(document.getElementById("googleMaps"), myOptions);
			  
			    var infoWindow = new google.maps.InfoWindow;

			    var onMarkerClick = function() {
			      var marker = this;
			      var latLng = marker.getPosition();
			      infoWindow.setContent('<h3>Marker position is:</h3>' +
				  latLng.lat() + ', ' + latLng.lng()+'<br><a href="#node10">Node 10</a>');

			      infoWindow.open(map, marker);
			    };
			    google.maps.event.addListener(map, 'click', function() {
			      infoWindow.close();
			    });

			    var marker1 = new google.maps.Marker({
			      map: map,
			      position: new google.maps.LatLng(38.736691,-9.302105)
			    });
			    var marker2 = new google.maps.Marker({
			      map: map,
			      position: new google.maps.LatLng(38.736642,-9.138671)
			    });
	
			    google.maps.event.addListener(marker1, 'click', onMarkerClick);
			    google.maps.event.addListener(marker2, 'click', onMarkerClick);
			}
		}
