			var contents = null;
			var node = new Array(13);

			function readFile() {
				var myUrl = "miavita.json";
				$.ajax({
				  url: myUrl,
				  context: document.body,
				  dataType: "text",
				  success: function(dataFileContent){
				    aux(dataFileContent);
				  }
				})
				retrieveDates();				
			}

			function aux(file){
				var myJson = new String(file);
				contents = jsonParse(myJson);
				fillContents();
			}

			function fillContents() {
				var size = Object.size(contents);

				for(var i = 0; i < 13; i++){
					node[i] = new Array();
				}
				for(var k in contents) {
					var index = contents[k].node_id;
					var packet = contents[k].sequence;					
					var i = 0;

					node[index].push(packet);
					node[index][packet] = new Array(9);

					node[index][packet][1] = contents[k].timestamp;
					node[index][packet][2] = contents[k].air_time;
					node[index][packet][3] = contents[k].sequence;
					node[index][packet][4] = contents[k].fails;
					node[index][packet][5] = contents[k].retries;
					node[index][packet][6] = contents[k].sample_1;
					node[index][packet][7] = contents[k].sample_2;
					node[index][packet][8] = contents[k].sample_3;
					node[index][packet][9] = contents[k].sample_4;
				}
			}

			function retrieveData(nodeID, parameter){
				var result = new Array();
				switch(parameter){
					case 'timestamp':
					  parameter = 1;
					  break;
					case 'air_time':
					  parameter = 2;
					  break;
					case 'sequence':
					  parameter = 3;
					  break;
					case 'fails':
					  parameter = 4;
					  break;
					case 'retries':
					  parameter = 5;
					  break;
					case 'sample1':
					  parameter = 6;
					  break;
					case 'sample2':
					  parameter = 7;
					  break;
					case 'sample3':
					  parameter = 8;
					  break;
					case 'sample4':
					  parameter = 9;
					//gps coordinates, temp, battery level
					  break;
					default:
					  alert("Holy Shit.");
				}
				
				for(var k in node[nodeID]){
					if(node[nodeID][k][parameter] != undefined)
						result.push(node[nodeID][k][parameter]);
				}
				
				return result;
			}

			function formatStackedData(retries, fails){
				var size = Object.size(retries);
				var data = new Array(size);

				for(var j = 0; j < size; j++){
					data[j] = new Array(2);
				}
				
				for(var i = 0; i < size; i++){
					data[i][0] = retries[i];
					data[i][1] = fails[i];
				}	
				return data;
			}
			
			function formatTooltipsBar(nodeID){
				var fails = retrieveData(nodeID, "fails");
				var retries = retrieveData(nodeID, "retries");
				var sequence = retrieveData(nodeID, "sequence");
				var size = Object.size(sequence) * 2;
				var result = new Array(size);
				var j = 0;

				for(var i = 0; j < size; j++){
					result[j] = new String("'Sequence: " + sequence[i] + ", Retries: " + retries[i] + "'");
					j++;
					if(j != size)
						result[j] = new String("'Sequence: " + sequence[i] + ", Fails: " + fails[i] + "'");
					i++;
				}

				return result;
			}

			function formatTooltipsLine(nodeID){
			
				var air_time = retrieveData(nodeID, "air_time");
				var sequence = retrieveData(nodeID, "sequence");
				var size = Object.size(sequence);
				var result = new Array(size);

				for(var j = 0; j < size; j++){
					result[j] = new String("Sequence: " + sequence[j] + ", Air_time: " + air_time[j]);
				}
				return result;
			}

			function formatTooltipsLineSysmology(nodeID){
				var sample1 = retrieveData(nodeID, "sample1");
				var sample2 = retrieveData(nodeID, "sample2");
				var sample3 = retrieveData(nodeID, "sample3");
				var size = Object.size(sample1);
				var result = new Array(size);

				for(var j = 0; j < size; j++){
					result[j] = new String("X-axis: " + sample1[j] + ", Y-axis: " + sample2[j] + ", Z-axis: " + sample3[j]);
				}
				return result;
			}

		
			function relabel(data){
				var size = Object.size(data);
				var result = new Array(size/20);
				var i = 0;

				for(var j = 0; j < size; j+=20){
					var tmp = new Date(data[j]*1000);
					var month = tmp.getMonth() + 1;
					result[i] = new String(tmp.getDate() + "/" + month + "@" + tmp.getHours() + ":" + tmp.getMinutes() + ":" + tmp.getSeconds());
					i++;
				}
				return result;
			}

			//network
			function draw(node){
				var xaxis = "timestamp";
				var yaxis = "air_time";
				var graph = "networkGraphNode" + node;
				
				var bar = new RGraph.Bar(graph, formatStackedData(retrieveData(node, "retries"), retrieveData(node, "fails")));
				//bar.Set('chart.title', "Node : " + node + " Coordinates: ");
				bar.Set('chart.ymax', 7);
				bar.Set('chart.colors', ['#ccc', 'green']);
				bar.Set('chart.background.grid.autofit', true);
				bar.Set('chart.grouping', 'stacked');
				//legendas para o eixo dos xx
				bar.Set('chart.labels', relabel(retrieveData(node, xaxis)));
				//bar.Set('chart.tooltips', ['as', 'asd', 'assdf', 'qw', 'qwe', 'rty']);
				bar.Set('chart.tooltips', formatTooltipsBar(node));
				//titulos para os eixos
				bar.Set('chart.title.xaxis', xaxis);
				bar.Set('chart.title.yaxis', yaxis);
				//espaço para legendas
				bar.Set('chart.gutter.bottom', 40); 
				bar.Set('chart.gutter.left', 40);
				bar.Set('chart.labels.above', true);
				//sombras
				bar.Set('chart.shadow', true);
				bar.Set('chart.shadow.color', '#bbb');

				var line = new RGraph.Line(graph, retrieveData(node, yaxis));
				
				line.Set('chart.background.grid.color', 'rgba(238,238,238,1)');
				line.Set('chart.colors', ['red']);
				line.Set('chart.tooltips', formatTooltipsLine(node));
				line.Set('chart.tooltips.effect', 'expand');
				line.Set('chart.linewidth', 2);
				line.Set('chart.tickmarks', 'circle');
				line.Set('chart.filled', false);

				bar.Set('chart.contextmenu', [
					['Zoom in', RGraph.Zoom],
					['Annotations', [['Enable', function(){line.Set('chart.annotatable', true);}],
							['Disable', function(){line.Set('chart.annotatable', false);}],
							//['Show palette', RGraph.Showpalette],
							['Refresh', function(){combo.Draw();}],
							['Clear', function(){
									RGraph.ClearAnnotations(line.canvas);	// Clear the annotation data
									RGraph.ClearAnnotations(bar.canvas);	// Clear the annotation data
									RGraph.Clear(line.canvas);		// Clear the chart
									RGraph.Clear(bar.canvas);		// Clear the chart
									combo.Draw();
				                                      }]]]
					]);

				bar.Set('chart.zoom.hdir', 'center');
				bar.Set('chart.zoom.vdir', 'center');

				//key
				bar.Set('chart.key', ['Retries', 'Fails']);
				bar.Set('chart.key.background', 'rgba(255,255,255,0.7)');
				bar.Set('chart.key.position.y', bar.Get('chart.gutter.top') + 5);
				bar.Set('chart.key.border', true);

				//efeitos
				if(networkShowEffects){
					//line.Set('chart.curvy', true); ahah stuff
					RGraph.Effects.Fade.In(bar);
					RGraph.Effects.Bar.Grow(bar);
					RGraph.Effects.Fade.In(line);
					RGraph.Effects.Line.UnfoldFromCenter(line);
				}

				var combo = new RGraph.CombinedChart(bar, line);

				combo.Draw();				
			}

		
			function drawSysmology(node){

				var graph = "sysmologyGraphNode" + node;
				var line = new RGraph.Line(graph, retrieveData(node, 'sample1'), retrieveData(node, 'sample2'), retrieveData(node, 'sample3'));
				//var line = new RGraph.Scatter(graph, data);
           			line.Set('chart.ymax', 130000);
				//line.Set('chart.ymin', -100);
				//line.Set('chart.scale.decimals', 1);
			        //line.Set('chart.xscale.decimals', 0);
			    	//line.Set('chart.tickmarks', 'circle');
			        //line.Set('chart.xscale', true);
				
				line.Set('chart.background.grid.color', 'rgba(238,238,238,1)');
				line.Set('chart.tooltips', formatTooltipsLineSysmology(node));
				//line.Set('chart.linewidth', 2);
				line.Set('chart.tickmarks', 'endcircle');
				line.Set('chart.filled', false);
				line.Set('chart.background.barcolor1', 'white');
				line.Set('chart.background.barcolor2', 'white');
				//posição relativa do eixo xx
				line.Set('chart.xaxispos', 'center');
				//titulos para os eixos
				line.Set('chart.title.xaxis', "timestamp");
				line.Set('chart.title.yaxis', "position");
				//espaço para legendas
				line.Set('chart.labels', relabel(retrieveData(node, "timestamp")));

				//line.Set('chart.hmargin', 40);
				line.Set('chart.gutter.left', 60);
				line.Set('chart.gutter.right', 60);
				line.Set('chart.gutter.bottom', 40); 
				//key
				line.Set('chart.key', ['X Axis', 'Y Axis', 'Z Axis']);
				line.Set('chart.key.background', 'rgba(255,255,255,0.7)');
				line.Set('chart.key.position.y', line.Get('chart.gutter.top') + 5);
				line.Set('chart.key.border', true);
				line.Set('chart.key.interactive', true);
				//line.Set('chart.scale.round', true);
//				line.Set('chart.variant', true);
				line.Set('chart.zoom.hdir', 'center');
				line.Set('chart.zoom.vdir', 'center');
				
				line.Set('chart.contextmenu', [
					['Zoom in', RGraph.Zoom],
					['Annotations', [['Enable', function() {line.Set('chart.annotatable', true);}],
							['Disable', function() {line.Set('chart.annotatable', false);}],
							//['Show palette', RGraph.Showpalette],
							['Refresh', function() {line.Draw();}],
							['Clear', function() {
									RGraph.ClearAnnotations(line.canvas);	// Clear the annotation data
									RGraph.Clear(line.canvas);		// Clear the chart
									line.Draw();
				                                      }]]]
					]);
				
				//efeitos
				if(sysmologyShowEffects){
					RGraph.Effects.Line.Unfold(line);
				}
				else {
					line.Set('chart.zoom.fade.in', true);
					line.Set('chart.zoom.fade.out', true);
				}
									
				line.Draw();
			}
			

