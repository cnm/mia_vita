
Object.size = function(obj) {
    var size = 0, key;
    for (key in obj) {
        if (obj.hasOwnProperty(key)) size++;
    }
    return size;
};


var sysmologyPresentTooltips = false;
var networkPresentTooltips = false;

var sysmologyShowEffects = false;
var networkShowEffects = false;


Array.prototype.max = function() {
    return Math.max.apply(null, this)
}

Array.prototype.min = function() {
    return Math.min.apply(null, this)
}

function init(){
    hideContent('config_network');
    hideContent('config_sysmology');
    hideContent('sysmology');
    hideContent('network');
    show('welcome');
    readFile();
    remove();
}


var maximumDate = 0;
var minimumDate = 0;

function retrieveDates(){
    var i = 1;

    for(; i <= 13; i++){
        var maxtmp = retrieveData(i, 'timestamp').max();
        if(maxtmp > maximumDate)
            maximumDate = maxtmp;
    }

    minimumDate = retrieveData(10, 'timestamp').min();
    /*for(i = 2; i <= 13; i++){
      var mintmp = retrieveData(1, 'timestamp').min(); //falta verificar para undefined etc JAVASCRIPT SUCKS
      if(mintmp < minimumDate)
      minimumDate = mintmp;
      }*/

    minimumDate = new Date(1320054399000);
    maximumDate = new Date(); //*1000 esta em milis

    $('#sysfrom').datetimepicker({
        minDate: minimumDate,
        maxDate: maximumDate,
        timeFormat: 'h:m',
        separator: ' @ ',
        onClose: function(dateText, inst) {
            var endDateTextBox = $('#sysfrom');
            if (endDateTextBox.val() != '') {
                var testStartDate = new Date(dateText);
                var testEndDate = new Date(endDateTextBox.val());
                if (testStartDate > testEndDate)
        endDateTextBox.val(dateText);
            }
            else {
                endDateTextBox.val(dateText);
            }
        },
        onSelect: function (selectedDateTime){
            var start = $(this).datetimepicker('getDate');
            $('#sysfrom').datetimepicker('option', 'minDate', new Date(start.getTime()));
        }
    });
    $('#systo').datetimepicker({
        minDate: minimumDate,
        maxDate: maximumDate,
        timeFormat: 'h:m',
        separator: ' @ ',
        onClose: function(dateText, inst) {
            var startDateTextBox = $('#systo');
            if (startDateTextBox.val() != '') {
                var testStartDate = new Date(startDateTextBox.val());
                var testEndDate = new Date(dateText);
                if (testStartDate > testEndDate)
        startDateTextBox.val(dateText);
            }
            else {
                startDateTextBox.val(dateText);
            }
        },
        onSelect: function (selectedDateTime){
            var end = $(this).datetimepicker('getDate');
            $('#systo').datetimepicker('option', 'maxDate', new Date(end.getTime()) );
        }
    });

    $( "#netfrom" ).datepicker( "option", "minDate", minimumDate );
    $( "#netto" ).datepicker( "option", "maxDate", maximumDate );
}

$(function() {
    var dates = $( "#netfrom, #netto" ).datepicker({
        defaultDate: "+1w",
        changeMonth: true,
        numberOfMonths: 1,
        onSelect: 
        function( selectedDate ) {
            var option = this.id == "netfrom" ? "minDate" : "maxDate",
        instance = $( this ).data( "datepicker" ),
        date = $.datepicker.parseDate(instance.settings.dateFormat || $.datepicker._defaults.dateFormat, selectedDate, instance.settings );

    dates.not( this ).datepicker( "option", option, date );
        }
    });
});

$(function(){
    $('#viewInfoPortal').portal({
        border:false,
        fit:true
    });
    $('#configNetworkPortal').portal({
        border:false,
        fit:true
    });
    $('#configSysmologyPortal').portal({
        border:false,
        fit:true
    });
    $('#sysmologyViewPortal').portal({
        border:false,
        fit:true
    });
    $('#networkViewPortal').portal({
        border:false,
        fit:true
    });
});

var sysmologyNodeSelected = false;
var networkNodeSelected = false;

function unSelectAll(obj){
    var selected;
    if(obj == 'sysmologyNode'){
        sysmologyNodeSelected = !sysmologyNodeSelected;
        selected = sysmologyNodeSelected;
    }
    else {
        networkNodeSelected = !networkNodeSelected;
        selected = networkNodeSelected;
    }

    for (i=1, content = '#' + obj + '1'; i<=13; i++, content = '#' + obj + i){
        $(content).prop("checked", selected);
    }
}

var contentShown = "#info";

function collapsePanel(panel){
    $(panel).panel('collapse'); //'#coco'
}

function remove(){
    $('#sysmologyViewPortal').portal('remove',$('#coco'));
    $('#sysmologyViewPortal').portal('resize');
}

function add(portal, node){

    //vir cá fora buscar o valor - doesn't matter which portal it is as they are equal!
    var width = $(networkViewPortal).width();
    var height = $(networkViewPortal).height();

    var p = $('<div/>').appendTo('body');
    if(contentShown == "#network")
        p.panel({
            title: 'Node ' + node,
            content:'<div id=networkPanelNode' + node + ' style="padding:10px;"><canvas id="networkGraphNode' + node + '" width="' + (width - 20) + '" height="' + (height*0.8) +'">[No canvas support]</canvas><br/></div>',
            height:(height*0.9),
            closable:false,
            collapsible:true
        });
    else
        p.panel({
            title: 'Node ' + node,
            content:'<div id=sysmologyPanelNode' + node + ' style="padding:10px;"><canvas id="sysmologyGraphNode' + node + '" width="' + (width - 20) + '" height="' + (height*0.8) +'">[No canvas support]</canvas><br/></div>',
            height:(height*0.9),
            closable:false,
            collapsible:true
        });

    $(portal).portal('add', {
        panel:p,
        columnIndex:0
    });

    $(portal).portal('resize');
}

function helper(){

    //for (j = 1, content = contentShown + 'GraphNode1'; j <= 13; j++, content = contentShown + 'GraphNode' + j){
    //	remove(contentShown + 'ViewPortal', content);
    //}

    for (i = 1, content = contentShown + 'Node1'; i <= 13; i++, content = contentShown + 'Node' + i){
        if ($(content).is(':checked')) {
            add(contentShown + "ViewPortal", i);
        }
    }

    sysmologyPresentTooltips = $("#sysmologyPresentTooltips").is(':checked');
    networkPresentTooltips = $("#networkPresentTooltips").is(':checked');

    sysmologyShowEffects = $("#sysmologyShowEffects").is(':checked');
    networkShowEffects = $("#networkShowEffects").is(':checked');

    for (counter = 1, content = contentShown + 'Node1'; counter <= 13; counter++, content = contentShown + 'Node' + counter){
        if ($(content).is(':checked')) {
            if(contentShown == "#network")
                draw(counter);
            else
                drawSysmology(counter);
        }
    }
}

function hideContent(obj){
    var content = "#" + obj;
    $(content).hide( "blind", {}, 1000, function(){});		
}

function show(obj){
    var content = "#" + obj;
    if(contentShown != content)
        $(contentShown).hide( "blind", {}, 1000, unhide(content));
}

function unhide(obj) {
    contentShown = obj;
    if(contentShown == "#map")
        googleInitialize();

    $(contentShown).removeAttr( "style" ).hide().fadeIn();

    if(contentShown == "#network" || contentShown == "#sysmology"){
        helper();
    }
};
