/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

define(['dojo/_base/declare',
        'dojo/_base/array',
		'dojo/dom',
		'dojo/number',
		'd3/d3.min',
		'dojo/i18n!ism/nls/chart',
		'idx/widget/HoverHelpTooltip',
		'ism/IsmUtils'
		], function(declare, array, dom, number, d3, nls, HoverHelpTooltip, Utils) {
	
	/*
	 * themeColors contain the starting and ending colors to use in a
	 * gradient (fill) plus the color to use for a line (stroke).
	 */
	var themeColors = { 
			blue: 	 {fill: ["#298FB5", "#1183AD"], stroke: "#0E698A", opacity: "1"},
			teal: 	 {fill: ["#4DBAB4", "#39b2ac"], stroke: "#33A09B", opacity: "1"},				
			green: 	 {fill: ["#709F41", "#60942C"], stroke: "#568528", opacity: "1"},
			yellow:  {fill: ["#D8AF19", "#D4A600"], stroke: "#BF9500", opacity: "1"},			
			orange:  {fill: ["#E48B42", "#e17e2d"], stroke: "#CA7128", opacity: "1"},				
			red: 	 {fill: ["#BA3E50", "#b2293d"], stroke: "#A02537", opacity: "1"},					
			magenta: {fill: ["#AF5597", "#a6428b"], stroke: "#953B7D", opacity: "1"},	
			purple:  {fill: ["#7B55B0", "#6C42A7"], stroke: "#613B96", opactiy: "1"},
			grey:    {fill: ["#bbbbbb", "#999999"], stroke: "#9A9A9A", opacity: "1"},
			blue50:    {fill: ["#298FB5", "#1183AD"], stroke: "#0E698A", opacity: ".5"},
            teal50:    {fill: ["#4DBAB4", "#39b2ac"], stroke: "#33A09B", opacity: ".5"},               
            green50:   {fill: ["#709F41", "#60942C"], stroke: "#568528", opacity: ".5"},
            yellow50:  {fill: ["#D8AF19", "#D4A600"], stroke: "#BF9500", opacity: ".5"},           
            orange50:  {fill: ["#E48B42", "#e17e2d"], stroke: "#CA7128", opacity: ".5"},               
            red50:     {fill: ["#BA3E50", "#b2293d"], stroke: "#A02537", opacity: ".5"},                   
            magenta50: {fill: ["#AF5597", "#a6428b"], stroke: "#953B7D", opacity: ".5"},   
            purple50:  {fill: ["#7B55B0", "#6C42A7"], stroke: "#613B96", opactiy: ".5"},
            grey50:    {fill: ["#bbbbbb", "#999999"], stroke: "#9A9A9A", opacity: ".5"}			
	};
	
	var colorIndex = ["blue", "teal", "green", "yellow", "orange", "red", "magenta", "purple", "blue", "teal", "green", "yellow", "orange", "red", "magenta", "purple"];
	
	/**
	 * Returns a linear gradient id using the specified themeColor.
	 */
	var getLinearGradient = function(themeColor, svg, containerId) {
		if (!svg) {
			return null;
		}
		
		if (!themeColors[themeColor]) {
			themeColor = "blue";
		} 
		if (!containerId) {
			containerId = "";
		}
		var id = themeColor+"_gradient_"+containerId;
		var gradient = svg.select("#"+id);
		if (!gradient.empty()) {
			return id;
		}
		var defs = svg.select("defs");
		if (defs.empty()) {
			defs = svg.append("defs");
		}
		gradient = defs.append("linearGradient")
		    .attr("id", id)
		    .attr("x2", "0%")
		    .attr("y2", "100%");

		gradient.append("stop")
		    .attr("offset", "0%")
		    .attr("stop-color", themeColors[themeColor].fill[0])
		    .attr("stop-opacity", themeColors[themeColor].opacity);

		gradient.append("stop")
		    .attr("offset", "100%")
		    .attr("stop-color", themeColors[themeColor].fill[1])
		    .attr("stop-opacity", themeColors[themeColor].opacity);
		return id;
	};
	
	/**
	 * Returns a radial gradient id using the specified themeColor.
	 */
	var getRadialGradient = function(themeColor, svg, id) {
		if (!svg) {
			return null;
		}
		
		if (!themeColors[themeColor]) {
			themeColor = "blue";
		} 
		if (!id) {
			id = themeColor+"_gradient";
		}
		var gradient = d3.select("#"+id);
		if (!gradient.empty()) {
			return id;
		}
		var defs = d3.select("defs");
		if (defs.empty()) {
			defs = svg.append("defs");
		}
		gradient = defs.append("radialGradient")
		    .attr("id", id)
		    .attr("x2", "0%")
		    .attr("y2", "100%");

		gradient.append("stop")
		    .attr("offset", "0%")
		    .attr("stop-color", themeColors[themeColor].fill[1]);

		gradient.append("stop")
		    .attr("offset", "100%")
		    .attr("stop-color", themeColors[themeColor].fill[0]);
		return id;
	};
	
	var getStrokeColor = function(themeColor) {
		if (!themeColors[themeColor]) {
			themeColor = "blue";
		} 
		return themeColors[themeColor].stroke;
	};

	var getOpacity = function(themeColor) {
        if (!themeColors[themeColor]) {
            themeColor = "blue";
        } 
        return themeColors[themeColor].opacity;
    };
    	
	/*=====
	var __ChartCtorArgs = {
		// summary:
		//		The keyword arguments that can be passed in a Chart constructor.
		// margin: Object?
		//		Optional margins for the chart, in the form of { l, t, r, b}.
		// width: Number? 
		//		Optional width in px. If not specified, defaults to 940.
		// height: Number?
		//	    Optional height in px. If not specified, defaults to 500.
		// chartData:  Array?
		//      Optional data to plot. If not specified, defaults to [].
		// xAxisOptions: Object?
		//      Optional xAxis options to define ticks and minorTicks. Both may be a number or an object {unit: unit, value: value}.
		// yAxisOptions: Object?
		//      Optional yAxis options. 
		//      max: If specified, sets an absolute max value for the y axis. If not specified, axis is dynamic based on domain.
		//      integers:  If true, use an integer formatter for the y axis values. Otherwise, use float.
		//      legend:  The label for the y axis.
		// brushMargin: Object?
		//      Optional margins for the zoom and pan brush control.  If not specified, brush control is not shown.
		// themeColor: String?
		//		Optional color to use, defaults to blue.  Must be a color defined in themeColors.
		// colors: Object?
		//      Optional colors to use for charts with more than one color.  Colors must be a color defined in themeColors. 
		//      The colors should be defined for each segment in the chart data.
		//      For example, for a pie chart with data segments "alpha" and "beta", the color object might look 
		// zoomBox: If true, support zooming by using the mouse to select the box to zoom in on. We're not currently using this...
		// radius: If provided, sets the radius for a pie chart.
		// TODO: THESE SHOULD BE REFACTORED SO YOU CANNOT SPECIFY MORE THAN ONE TYPE OF CHART.. The default is to show an area chart
		// stacked:  If true, show a stacked area chart
		// pie: If true, show a pie chart
		// bar: If true, show a bar chart
		// mirror: If true show a mirrored bar chart		
	};
	=====*/

	var Chart = declare("ism.widgets.charting.Chart", null, {
		// summary:
		// 		D3: Simple area or stacked area chart with linear x and y axis; Pie chart; Bar chart;
		//
		containerNodeId: undefined,
		margin: {top: 10, right: 20, bottom: 10, left: 10},
		width: 940,
		height: 500,
		svg: undefined,
		chartData: [],
		xAxis: undefined,
		yAxis: undefined,
		yAxisLabel: undefined,
		xAxisOptions: {},
		yAxisOptions: {},
		themeColor: "blue",
		rendered: false,
		stacked: false,
		pie: false,
		bar: false,
		mirror: false,
		radius: 70,
		scaleYAxis: true,
		showBrush: false,
		nls: nls,
		defaultYTicks: 8,
		tickUnit: undefined,
		tickValue: undefined,
		minorTickUnit: undefined,
		minorTickValue: undefined,
		zoomBox: false,
				
		constructor: function(/* DOMNode */containerNodeId, /* __ChartCtorArgs? */kwArgs) {
			if (kwArgs) {
				this.margin = kwArgs.margin ? kwArgs.margin : this.margin;
				this.width = (kwArgs.width ? +kwArgs.width : this.width) - this.margin.left - this.margin.right;
				var totalHeight = kwArgs.height ? +kwArgs.height : this.height;
				if (totalHeight < 300) {
					this.defaultYTicks = 6;
				}
				this.height = totalHeight - this.margin.top - this.margin.bottom;
				this.chartData = kwArgs.chartData ? kwArgs.chartData : [];
				this.xAxisOptions = kwArgs.xAxisOptions ? kwArgs.xAxisOptions : {};
				this.yAxisOptions = kwArgs.yAxisOptions ? kwArgs.yAxisOptions : {};
				this.themeColor = kwArgs.themeColor ? kwArgs.themeColor : "blue";
				this.stacked = kwArgs.stacked;
				this.pie = kwArgs.pie;
				this.bar = kwArgs.bar;
				this.mirror = kwArgs.mirror;
				this.zoomBox = kwArgs.zoomBox;
				this.radius = kwArgs.radius ? kwArgs.radius : Math.min(this.width, this.height)/2;
				this.colors = kwArgs.colors ? kwArgs.colors : {};
				this.scaleYAxis = this.yAxisOptions.max === undefined;  // no max set, scale it...
				this.showBrush = kwArgs.brushMargin ? true : false; // if brushMargin is specified, include a brush
				this.brushMargin = kwArgs.brushMargin;
				this.brushHeight = this.brushMargin ? totalHeight - this.brushMargin.top - this.brushMargin.bottom : undefined;
				// can be defined a an object or an integer
				if (this.xAxisOptions.ticks) {
					if (typeof this.xAxisOptions.ticks === 'number') {
						this.tickValue = this.xAxisOptions.ticks;
					} else {
						this.tickUnit = this.xAxisOptions.ticks.unit;
						this.tickValue = this.xAxisOptions.ticks.value;						
					}
				}
				if (this.xAxisOptions.minorTicks) {
					if (typeof this.xAxisOptions.minorTicks === 'number') {
						this.minorTickValue = this.xAxisOptions.minorTicks;
					} else {
						this.minorTickUnit = this.xAxisOptions.minorTicks.unit;
						this.minorTickValue = this.xAxisOptions.minorTicks.value;						
					}
				}

			}
			this.containerNodeId = containerNodeId;
			this.rendered = false;
			
			this._setup();
			
		},		
			
		// This is just adding the chartData right now, we only have one series. The name is not currently used.
		// chartData is the data to plot. It is always an array of objects. The format of the object varies with the type of chart.
		//    pie:  {color: color, dataItem: name, legend: legendText, legendLabel: label, text: formattedValue, y: value}
		//    area/bar chart: {date: Date, value: Number}
		//    stacked areas: {date: Date, value: {<nameOfSegment>: Number} }
		//    mirrored bars: {date: Date, value: Number, mirrorValue: Number}
		// seriesMap is required if you plan to use a checkbox legend with a stacked area chart. seriesMap is an object like 
		//    {<nameOfSegment>: {legend: legend, included: included} }. Legend is used in the hover behavior and included is
		//    used to determine if a segment is included in the plot or not.
		addSeries: function(/* String */name, /* Array */chartData, /*Object*/seriesMap) {
		    // for now it's the same as updateSeries
		    this.updateSeries(name, chartData, seriesMap);
		},
		
		updateSeries: function(/* String */name, /* Array */chartData, /*Object*/seriesMap) {
		    // if we already have data, don't immediately update until rendering it
            if (this.chartData) {
                this.newChartData = chartData;
            } else {
                this.chartData = chartData;
            }
               
            if (seriesMap) {
                if (this.seriesMap) {                   
                    this.newSeriesMap = seriesMap;
                } else {
                    this.seriesMap = seriesMap;
                }
            }
		},	
		
		setScaleYAxis: function(/*boolean*/ scaleYAxis) {
			this.scaleYAxis = scaleYAxis;
		},
		
		// Add legend with optional checkboxes. Only supported for stacked, pie, and mirror charts.
		// Checkboxes are only supported for stacked area charts.
		// legendNodeId is the id of the node to place the legend.
		// topic is the topic to publish on when a checkbox state is changed
		// options is an object that contains the following:
		//     legend: An object containing the text properties for each checkbox. The name of each label must correspond to the chartData.
		//             The legend may optionally contain an object named 'hover' which contains the hover text for each checkbox.
		//             The legend may optionally contain an object named 'subgroup' which defines a subgrouping of checkboxes and a 
		//             label for the subgroup (e.g. subgroup: { label: "Records", items: ["item1, "item2"] }. Only 1 subgroup is 
		//             supported and only for vertical legends.
		//     checkboxes: Whether to show checkboxes for a stacked area chart legend
		//     horizontal: Whether to render the legend horizontally or vertically
		addLegend: function(legendNodeId, topic, options) {
			if (!legendNodeId || !(this.stacked || this.pie || this.mirror )) {
				return;
			}
			
			var t = this;
			t.legendNodeId = legendNodeId;			
			options = options ? options : {};
			t.legendOptions = options;
			var legendText = options.legend ? options.legend : {};
			if (!legendText.hover) {
				legendText.hover = {};
			}
			var checkboxes = options.checkboxes && topic && t.seriesMap;	
			var styleClass = options.horizontal ? "horizontalLegend" : "verticalLegend";

			// create a legend for each element in the domain
			var data = t.color.domain().slice();
			if (t.stacked) {
				data.reverse(); // reverse the legend to match the stacked areas
			}
			
			// subgroup?
			var subgroup = legendText.subgroup ? legendText.subgroup : [];
			if (subgroup.length > 0) {
				t.legendNodeSubgroupId = [];
				for (var n=0, cnt=subgroup.length; n < cnt; n++) {
					var items =  subgroup[n].items ? subgroup[n].items : [];
					if (items.length > 0 && styleClass === "verticalLegend") {
					    // split the data according to the subgroup
					    var mainData = data;
					    var subGroupData = [];
					    for (var i = 0, len=items.length; i < len; i++) {
					        var index = array.indexOf(mainData, items[i]);
					        if (index < 0) {
					            continue;
					        }
					        subGroupData.push(items[i]);
					        mainData.splice(index,1);
					    }
					    subGroupData.reverse();
					    subgroup[n].subGroupData = subGroupData;
					}
				}
			    // do the mainData
                t._createLegendElements(legendNodeId, data, styleClass, topic, t, checkboxes, legendText);              
                // now the subgroup
				for (n=0, cnt=subgroup.length; n < cnt; n++) {
	                // id for the subgroup
	                t.legendNodeSubgroupId[n] = t.legendNodeId+"_subgroup_"+n;
	                // create a fieldset with this id
	                var groupFieldset = d3.select("#"+t.legendNodeId).append("fieldset")
	                           .attr("class", "legendSubgroup " + styleClass)
	                           .attr("id", t.legendNodeSubgroupId[n]);
	                // add a label 
	                groupFieldset.append("legend").attr("align","center").text(subgroup[n].label);
	                // add a placeholder for a horizontal line
	                groupFieldset.append("div").attr("id", t.legendNodeId+"_horizontalLine");
	                // add the items
	                t._createLegendElements(t.legendNodeSubgroupId[n], subgroup[n].subGroupData, styleClass, topic, t, checkboxes, legendText);
				}
			} else {
	            t._createLegendElements(legendNodeId, data, styleClass, topic, t, checkboxes, legendText);			    
			}
			
		    t._addHorizontalLineLegend(t);
		},
		
	    _createLegendElements: function(legendNodeId, data, styleClass, topic, t, checkboxes, legendText) {
              // Append a div for each element in the domain
            // The div gets the hover help, if any, plus up to 4 spans (one for hover help, one for the checkbox
            // one for the graphic and one for the labels)
            var divs = d3.select("#"+legendNodeId).selectAll(".legendDiv")
                            .data(data)
                        .enter().append("div")
                            .attr("class", "legendDiv " + styleClass)
                            .attr("id", function(d) {return legendNodeId+"_"+d; })
                            .each( function(d) {
                            	if (legendText.hover[d]) {
                            		// Explicitly set title to undefined to avoid inheriting 
                            		// parent node title.
                            	    d3.select(this).attr("title", function(d) { return ""; });
                    				var tooltip = new HoverHelpTooltip({
                    					connectId: [legendNodeId+"_"+d],
                    					forceFocus: true, 
                    					showLearnMore:false,
                    					position: ["below"],
                    					hideDelay: 100,
                    					label: legendText.hover[d]
                    				});
                    				tooltip.startup();
                            	}
                            });

            if (checkboxes) {
                // create the check boxes
                divs.append("span")
                    .attr("class", "legendCheckbox")
                    .append("input")
                        .attr("type", "checkbox")
                        .attr("role", "checkbox")
                        .property("checked", function(d) {return t.seriesMap[d].included; })
                        .attr("value", function(d) { return d; })
                        .attr("aria-label", function (d) { return legendText[d] ? legendText[d] : d; } )
                        .on("change", function() {
                            console.log("Publishing message to " + t.topic + "checked: " + this.checked +", name: " + this.value);
                            dojo.publish(topic, {checked: this.checked, name: this.value});
                        });
            }
            
            // add the legends
            var svg = divs.append("span")
                    .attr("class", "legend")
                        .append("svg")
                            .attr("width", 16)
                            .attr("height", 16)
                            .append("g")
                                .attr("transform", "translate(" + 0 + "," + 0 + ")");           
            
            // draw the legend 'box' and give it the right colors
            svg.append("rect")
              .attr("x", 0)
              .attr("width", 16)
              .attr("height", 16)
              .style("fill", function(d) { return "url(#"+getLinearGradient(t.colors[d], svg, t.legendNodeId)+")"; })
              .style("stroke", function(d) { return getStrokeColor(t.colors[d]); })
              .style("stroke-opacity", function(d) { return getOpacity(t.colors[d]); }); 

           // add the legend text (add as regular text for g11n expansion)
           t.legend = divs.append("div")
                    .attr("class", "flexChartLegend legendText")                    
                    .text(function(d) { return legendText[d] ? legendText[d] : d; });

        },
        
		/**
		 * Adds a horizontal line to the graph at the specified yValue in the specified color
		 * Options is an object containing"
		 * Number: yValue A number on the y-axis
		 * String: color  A color (if not a theme color, must be a valid svg color name. 
		 *                See http://www.w3.org/TR/SVG/types.html#ColorKeywords)
		 * Number: width  The width of the line in pixels
		 * String: legend The text to show in the legend (optional)
		 * String: legendHover The text to show when hovering over the item in the legend (optional)
		 */
		addHorizontalLine: function(options) {
		    if (!options || options.yValue === undefined) {
		        return;
		    }
		    var color = options.color;
		    var width = options.width;
		    if (color) {
		        color =  themeColors[color] ? getStrokeColor(themeColors[color]) : color;
		    } else {
		        color = "darkslategray";
		    }
		    if (width === undefined) {
		        width = 1;
		    }
		    width = width+"px";
		    this.horizontalLineOptions = {yValue: options.yValue, color: color, width: width, legend: options.legend, legendHover: options.legendHover};
		    
		    if (this.rendered) {
		        this._createHorizontalLine(this);
		        if (options.legend && this.legend) {
		            this._addHorizontalLineLegend(this);
		        }
		    }
		},
		
		
		_addHorizontalLineLegend: function(context) {
		    var t = context ? context : this;
		    
		    if (!t.horizontalLineOptions || !t.horizontalLineOptions.legend || !t.legendNodeId) {
		        return;
		    }
		    
	        var styleClass = t.legendOptions && t.legendOptions.horizontal ? "horizontalLegend" : "verticalLegend";
	        var title = t.horizontalLineOptions.legendHover ? t.horizontalLineOptions.legendHover : ""; 
		    
	        // determine where to put the line. If we have a subgroup, put it in the subgroup,
	        // otherwise, put it at the end of the legend

	        var hlDiv;
	        if (t.legendNodeSubgroupId) {
	            hlDiv = d3.select("#"+t.legendNodeId+"_horizontalLine");
	        } else {
	            hlDiv = d3.select("#"+t.legendNodeId).append("div").attr("id", t.legendNodeId+"_horizontalLine");
	        }
	        
		    // add a div for the hl legend
            hlDiv.attr("class", "legendDiv " + styleClass).property("title", title);
            
            // add a span with an svg element for the svg part
            var svg = hlDiv.append("span")
                       .attr("class", "legend")
                       .append("svg")
                       .attr("width", 18)
                       .attr("height", 16)
                       .append("g")
                           .attr("transform", "translate(" + 0 + "," + 0 + ")");
           
           // draw the legend 'line' and give it the right colors
           svg.append("line")
               .attr("x1", 0)
               .attr("y1", 8)
               .attr("x2", 16)
               .attr("y2", 8)
               .style("stroke", t.horizontalLineOptions.color)
               .style("stroke-width", t.horizontalLineOptions.width);
           
           // add the legend text (add as regular text for g11n expansion)
           hlDiv.append("div")
                 .attr("class", "flexChartLegend legendText")                    
                 .text(t.horizontalLineOptions.legend);
        		    
		},
		
		_updatePieLegend: function(legends) {
			var t = this;
			if (!t.legend || !legends) {
				return;
			}
			t.legend.text(function(d) {	return legends[d]; });			
		},
				
		_integerFormatter: function(n, i) {
			// Note, operates on absolute value of n due to the mirror bar graph function
			return number.format(Math.abs(n), {places: 0, locale: ism.user.localeInfo.localeString});
		},
		
		_getFloatFormatter: function(num, intervals) {			
			var n = num === 0 ? 1 / intervals : num / intervals;
			
			if (Math.floor(n) == n || (intervals > 1 && n > 1)) {
				return this._integerFormatter;
			} else if (n >= 0.1) {
				return this._floatFormatter1;
			}
			
			var floatFormatter = this._floatFormatter2;
			if (n < 0.0001) {
				floatFormatter = this._floatFormatter5;
			} else if (n < 0.001) {
				floatFormatter = this._floatFormatter4;
			} else if (n < 0.01){
				floatFormatter = this._floatFormatter3;					
			} 			
			return floatFormatter;
		},
		
		_floatFormatter1: function(n, i) {
			return number.format(n, {places: 1, locale: ism.user.localeInfo.localeString});			
		},

		_floatFormatter2: function(n, i) {
			return number.format(n, {places: 2, locale: ism.user.localeInfo.localeString});			
		},

		_floatFormatter3: function(n, i) {
			return number.format(n, {places: 3, locale: ism.user.localeInfo.localeString});			
		},
		
		_floatFormatter4: function(n, i) {
			return number.format(n, {places: 4, locale: ism.user.localeInfo.localeString});			
		},
		
		_floatFormatter5: function(n, i) {
			return number.format(n, {places: 5, locale: ism.user.localeInfo.localeString});			
		},
		
		_dateFormatter: function(d, i) {
			return Utils.getFormattedTime(d);
		},
		
		_dateFormatterSeconds: function(d, i) {
			return Utils.getFormattedTime(d, true);
		},
		
		_setup: function () {
			var t = this;
			
			var svg = d3.select("#"+t.containerNodeId).append("svg")
				.attr("width", t.width + t.margin.left + t.margin.right)
				.attr("height", t.height + t.margin.top + t.margin.bottom);
						
			t.svg = svg.append("g")
	  			.attr("transform", "translate(" + t.margin.left + "," + t.margin.top + ")");
			
			t.svg.append("defs").append("clipPath")
	    			.attr("id", t.containerNodeId+"_clip")
	    		.append("rect")
	    			.attr("width", t.width)
	    			.attr("height", t.height);

			if (t.pie) {
				t._setupPie();
				return;
			}
			
			// time x and linear y axes
			t.x = d3.time.scale().range([0, t.width]);
			t.y = d3.scale.linear().range([t.height, 0]);			
			t.xAxis = d3.svg.axis().scale(t.x).orient("bottom");
			
			t._initializeTicks(t);
			
			t.xAxis.outerTickSize(0);
						
			t.yAxis = d3.svg.axis()
				.scale(t.y)
				.orient("left");
			t.yAxis.ticks(t.defaultYTicks);			
			t.yAxis.outerTickSize(0);
			t.yAxis.tickFormat(t.yAxisOptions.integers ? t._integerFormatter : t._floatFormatter);
					
			
			// Single plot area chartData is assumed to be an array of objects like {date: Date, value: Number}
			// Stacked areas is assumed to be like {date: Date, value: {nameOfSegment: Number} }
			if (!t.bar) {
				t.area = d3.svg.area()
					.x(function(d) { return t.x(d.date); })
					.y0(t.stacked ? function(d) { return t.y(d.y0); } : t.height)
					.y1(function(d) { return t.stacked ? t.y(d.y0 + d.y) : t.y(d.value); });
			}
			
			if (t.stacked) {
				t.stack = d3.layout.stack()
					.values(function (d) { return d.values; });
				//t.color = d3.scale.category20();  
				t.color = d3.scale.ordinal();  // custom gradients
			} 
			
			if (t.mirror) {
				t.color = d3.scale.ordinal();  // custom gradients
				t.color.domain(["value","mirrorValue"]); //for legend
				t.colors.value = t.themeColor;
				t.colors.mirrorValue = t.mirror.color;
			}
			
			if (t.showBrush) {			
				t.brushSvg = svg.append("g")
		  			.attr("transform", "translate(" + t.brushMargin.left + "," + t.brushMargin.top + ")");
				
				// create the x, y, and x-axis for the brush graph
				t.brushX = d3.time.scale().range([0, t.width]);
				t.brushY = d3.scale.linear().range([t.brushHeight, 0]);
				t.brushXAxis = d3.svg.axis().scale(t.brushX).orient("bottom");
				if (t.tickUnit && t.tickValue) {
					t.brushXAxis.ticks(d3.time[t.tickUnit], t.tickValue);
				} else {
					t.brushXAxis.ticks(t.tickValue ? t.tickValue : 10);
				}				
				t.brushXAxis.outerTickSize(0);
				t.brushXAxis.tickFormat(t._dateFormatter);

				// create the brush
				t.brush = d3.svg.brush().x(t.brushX).on("brush", function(){
				    //console.debug("brush: empty? " + t.brush.empty() +", extent: " + t.brush.extent() + ", domain: " + t.brushX.domain());
					// modify the domain based on the brush extent
					t.x.domain(t.brush.empty() ? t.brushX.domain() : t.brush.extent());
					// update the main chart area and x-axis for the new domain
					if (!t.stacked) {
						t.svg.select("path.area").attr("d", t.area);
					} else {
						t.svg.selectAll("path.area")
							.attr("d", function(d, i) { return t.area(d[i].values); });
					}
					t._adjustXAxis(t);
					// update y domain and axis to allow scale to be specific to new x domain?					
				});
				// create the area for the brush
				t.brushArea = d3.svg.area()
					.x(function(d) { return t.brushX(d.date); })
					.y0(t.brushHeight)
					.y1(function(d) { return t.stacked ? t.brushY(d.total) : t.brushY(d.value); });
			}						
		},
		
		_adjustXAxis: function(t) {
		    console.debug(t.containerNodeId+" _adjustXAxis");
            var zoom = t.brush && !t.brush.empty();  // are we zooming?
            var overrides = undefined;
            if (zoom && t.width < 600) {
                // on narrow graphs with certain minute ranges, D3 decides to cram too much in
                var seconds = (t.brush.extent()[1].getTime() - t.brush.extent()[0].getTime())/1000;
                var minutes = seconds / 60;
                if (minutes < 30) {
                    if (minutes >= 15) {
                        overrides = {tickUnit: "minute", tickValue: 5, minorTickUnit: "second",  minorTickValue: 30};                                                           
                    } else if (minutes > 6) {
                        overrides = {tickUnit: "minute", tickValue: 2, minorTickUnit: "second",  minorTickValue: 15};
                    } else if (minutes > 3) {
                        overrides = {tickUnit: "minute", tickValue: 1, minorTickUnit: "second",  minorTickValue: 15};   
                    } else if (seconds > 60) {
                        overrides = {tickUnit: "second", tickValue: 30, minorTickUnit: "second", minorTickValue: 5};    
                    } else if (seconds > 30) {
                        overrides = {tickUnit: "second", tickValue: 15, minorTickUnit: "second", minorTickValue: 5};                                    
                    }  else if (seconds > 7) {
                        overrides = {tickUnit: "second", tickValue: 2, minorTickUnit: "second", minorTickValue: 1};                                 
                    } else {
                        overrides = {tickUnit: "second", tickValue: 1, minorTickUnit: "second", minorTickValue: 0.5};                                                                   
                    }
                } 
            } else {
                // format dates with seconds?
                var overrideSeconds = false;
                if (zoom) {
                    var range = t.brush.extent();
                    if (range[1] - range[0] < 600000) {
                        overrideSeconds = true;
                    }
                }
                // reset the ticks
                t._initializeTicks(t, overrideSeconds);                     
            }
            t._updateXAxis(t, zoom, overrides);
		},
		
		_setupPie: function() {
			// Pie is assumed to be an array of objects like (inherited from FlexChart)
			//     {color: color, dataItem: name, legend: legendText, 
			//      legendLabel: label, text: formattedValue, y: value}
			var t = this;
			
			t.color = d3.scale.ordinal();
			
			t.arc = d3.svg.arc()
				.innerRadius(0)
			    .outerRadius(t.radius - 10)
	    			.startAngle(function(d) { return d.startAngle + Math.PI/2; })
	    			.endAngle(function(d) { return d.endAngle + Math.PI/2; });

			t.pie = d3.layout.pie()
			    .sort(null)
			    .value(function(d) { return d.y; });
			
			// for when we need an empty circle
			t.emptyPie = t.svg.append("g")
		      .attr("class", "emptyPie")
		      .style("display", "none");			
			t.emptyPie.append("circle")
			  .attr("r", t.radius - 10);

		},
		
		_renderPie: function() {
			// chartData looks like:
			//     {color: color, dataItem: name, legend: legendText, 
			//      legendLabel: label, text: formattedValue, y: value}						
			var t = this;	

			// any non-zero slices?
			var nonZeroSlice = false;
			var colorDomain = [];
			var legends = {};
			for (var i in t.chartData) {
				if (t.chartData[i].y > 0) {
					nonZeroSlice = true;					
				}
				var name = t.chartData[i].dataItem;
				colorDomain.push(name);
				t.colors[name] = t.chartData[i].color;
				legends[name] = t.chartData[i].legend;
			}

			t.color.domain(colorDomain);
			
			if (t.rendered) {
				// update the slices 
				t.piePath = t.piePath.data(t.pie(t.chartData));
				t.piePath.attr("d", t.arc);
			 	t.piePath.attr("stroke-opacity", function(d, i) { return d.data.y > 0 ? "1.0" : "0.0"; });
				
				// update titles
				t.piePath.select("title").text(function(d, i) {	return d.data.y > 0 ? t.chartData[i].legend : ""; });
				
				// update legend
				t._updatePieLegend(legends);
				
				if (!nonZeroSlice) {
					// show the empty pie
					t.emptyPie.style("display", null);
				} else {								
					t.emptyPie.style("display", "none");
				}

				return;
			}

			if (!nonZeroSlice) {
				// show the empty pie
				t.emptyPie.style("display", null);
				return;
			}					
			
			t.emptyPie.style("display", "none");
			
			t.piePath = t.svg.datum(t.chartData).selectAll("path")
					.data(t.pie)
				 .enter().append("path")
				 	.attr("fill", function(d, i) { return "url(#"+getRadialGradient(t.chartData[i].color, t.svg)+")"; })
				 	.attr("stroke", function(d, i) { return getStrokeColor(t.chartData[i].color); })
				 	.attr("stroke-opacity", function(d, i) { return d.data.y > 0 ? "1.0" : "0.0"; })
				 	.attr("d", t.arc);
			 
			t.piePath.append("title").text(function(d, i) {	return d.data.y > 0 ? t.chartData[i].legend : ""; });

			// update legend  			t._updatePieLegend(legends);

			t.rendered = true;			
		},
		
		render: function() {
			var t = this;
			
			var brushExtent = t.brush && !t.brush.empty() ? t.brush.extent() : undefined;
			
            if (t.newChartData) {
                t.chartData = t.newChartData;
                t.newChartData = undefined;
            }
            if (t.newSeriesMap) {
                t.seriesMap = t.newSeriesMap;
                t.newSeriesMap = undefined;
            }

			if (t.pie) {
				t._renderPie();
				return;
			}
			            
			var seriesMap = t.seriesMap ? t.seriesMap : {};

			// set the x-axis domain
			var min = t.chartData.length > 0 ? t.chartData[0].date : 0;
			var max = t.chartData.length > 1 ? t.chartData[t.chartData.length-1].date : 0;
			var skipXAxisUpdate = false;
			if (min && max) {
			    var domain = [min,max];
				// are we zoomed in?
                if (brushExtent) {
                    // we need to see if the brush extent is still in the domain
                    if (min > brushExtent[0]) {
                        console.debug(t.containerNodeId+" need to adjust brush");
                        if (min > brushExtent[1]) {
                            // it's totally out, we need to give up on it
                            brushExtent = [0,0];
                        } else {
                            // it's part way out, we can adjust it
                            brushExtent[0] = min;                           
                        }
                    } else {
                        skipXAxisUpdate = true; // keeping our current brush extent so axis should remain static 
                    }
                    // otherwise it's all in, no adjustment needed...
                    // if we still have a brush extent, the domain is the brush extent 
                    if (!t.brush.empty()) {
                        domain = brushExtent;
                    }
                }
                
                // update the x domain 
                t.x.domain(domain);
			}

			if (t.stacked) {
				array.forEach(t.chartData, function(d, i) {
					var total = 0;
					for (var prop in d.value) {
						if (seriesMap[prop] && seriesMap[prop].included === false) {
							continue;
						}
						total += d.value[prop];
					}
					d.total = total;
				});
			}
		
			// set the y-axis domain
			var maxY = (t.yAxisOptions.max && !t.scaleYAxis)? t.yAxisOptions.max : 
				d3.max(t.chartData, function(d) { 
					if (t.stacked) {
						return d.total;
					} else if (t.mirror) {
						// for mirror, we want the 0 in the middle
						return Math.max(d.value, d.mirrorValue); 
					} 
					return d.value; 
				});
			
			if (!t.yAxisOptions.integers) {
				// do we need to use a finer grained formatter?
				t.yAxis.tickFormat(t._getFloatFormatter(maxY, 10));
			}
			
			maxY = maxY === 0 ? 1 : maxY;
			var minY = t.mirror ? -1 * maxY : 0; 
			t.y.domain([minY, maxY]);
			console.log("setting y domain to [" + minY + ", " + maxY + "]");
			var ticks = t.yAxisOptions.integers ? Math.min(t.mirror ? maxY*2 : maxY, t.defaultYTicks) : t.defaultYTicks;
			t.yAxis.ticks(ticks);
			
			if (t.showBrush) {
				// update the brush domain
				t.brushX.domain([min,max]);
				t.brushY.domain([0, maxY]);
			}
			
			if (t.stacked && t.chartData.length > 0) {
				// Set the color domain.  The color domain contains the name of the segments
				var colorDomain = [];
				var exemplar = t.chartData[0];
				var index = 0;
				for (var prop in exemplar.value) {
					colorDomain.push(prop);
					if (!t.colors[prop]) {
						t.colors[prop] = colorIndex[index];
					}
					index++;
				}
				t.color.domain(colorDomain);
				// Map the categories
				t.categories = t.stack(t.color.domain().map(function(name) {
					return {
						name: name,
						values: t.chartData.map(function(d) {
							// If we have a series map, return 0 if the 
							// named element is not currently included in the stack
							var value = d.value[name];
							if (seriesMap[name] && seriesMap[name].included === false) {
								value = 0;
							}
							return { date: d.date, y: value };
						})
					};
				}));
			}				
			
			if (t.rendered) {
				if (t.resetZoomLink) {
					t.resetZoomLink.style("display", "none");
				}
				
				// update the axes (y first since it can change margin)
				t._updateYAxis(t);
				if (t.showBrush) {
				    if(!skipXAxisUpdate) {
				        t._adjustXAxis(t);
				    }
				} else {
				    t._updateXAxis(t);
				}
				
				if (t.stacked) {
					t._updateStackedElements(t);
				} else if (t.bar) {
					t._updateBarElements(t);
				} else {
					t._updateAreaElements(t);
				}
								
				if (t.showBrush) {
					// update brushXAxis
					t.brushSvg.selectAll(".minor").remove();  // remove minor tick lines
					t.brushSvg.select(".brushX.axis").call(t.brushXAxis); // refresh x axis
					if (!t.brush.empty() && brushExtent) {
					    t.brush.extent(brushExtent);					
					    t.brushSvg.select(".brush").call(t.brush); // redraw the brush
					}
                    t._createMinorTicks(t, t.brushXAxisG, t.brushX);
				}
				
				var y = 0;
				if (t.zeroLine) {
				    y = t.y(0);
				    t.zeroLine.attr("y1", y)
				    	.attr("y2", y);										
				}
				
                if (t.horizontalLine) {
                    y = t.y(t.horizontalLineOptions.yValue);
                    t.horizontalLine.attr("y1", y)
                        .attr("y2", y);                                        
                }
				
				return;
			}
			
			// not yet rendered...
			
			if (t.stacked) {
				t._createStackedElements(t);
			} else if (t.bar) {
				t._createBarElements(t);
			} else {
				t._createAreaElements(t);
			}

			// create the y-axis (with label)
			var text = t.svg.append("g")
			      .attr("class", "y axis")
			    .append("text")
			      .attr("transform", "rotate(-90)") // vertical text
			      .attr("y", -1*t.margin.left+20) // left of the y-axis by margin.left-20px
			      .attr("x", -1*(t.height/2)) // center on the y-axis
			      .style("text-anchor", "middle") // center the text around x,y
			      .style("font-size", "13px")
			      .text(t.yAxisOptions.legend);
			
		    t._wrapString(text, t.yAxisOptions.legend, t.height);	 // y-axis label		    
		    t.yAxisLabel = text;
		    t._updateYAxis(t);
			
			// create the x-axis (no label)
			t.xAxisG = t.svg.append("g")
		      .attr("class", "x axis")
		      .attr("transform", "translate(0," + t.height + ")")
		      .call(t.xAxis);
			
			// create minor ticks
			t._createMinorTicks(t, t.xAxisG, t.x);
			
			if (t.showBrush) {
				// create the x-axis (no label)
				t.brushXAxisG = t.brushSvg.append("g")
			      .attr("class", "brushX axis")
			      .attr("transform", "translate(0," + t.brushHeight + ")")
			      .call(t.brushXAxis);
				
			    var brushText = t.brushXAxisG.append("text")
				      .attr("y", 40) 
				      .attr("x", t.width/2) // center on the x-axis
				      .style("text-anchor", "middle") // center the text around x,y
				      .attr("class", "brushTitle")
				      .text(nls.brushTitle);
			    
			    t._wrapString(brushText, nls.brushTitle, t.width, true);
				
				// create minor ticks
				t._createMinorTicks(t, t.brushXAxisG, t.brushX);
			}

			if (t.mirror) {
				// add a zero line
				t.zeroLine = t.svg.append("line")
					.attr("class", "mirrorZeroLine")
					.attr("x1", 0)
					.attr("y1", t.y(0))
					.attr("x2", t.width)
					.attr("y2", t.y(0));					
			}
            t._createHorizontalLine(t); 
			
			// show zoom text if zoomBox
			if (t.zoomBox) {
				t.xAxisG.append("text")
			      .attr("y", 40) 
			      .attr("x", t.width/2) // center on the x-axis
			      .style("text-anchor", "middle") // center the text around x,y
			      .attr("class", "zoomTitle")
			      .text(nls.zoomControl.title);
								
				dojo.subscribe(this.containerNodeId+"resetZoom", function(t) {
						t.resetZoomLink.style("display", "none");					
						t._initializeTicks(t);
						t.render();
					});
			}			
			
			t._createMouseBehavior(t);
			
			t.rendered = true;
		},
		
		destroy: function() {
			console.log("Chart destroy called");
			d3.select("#"+this.containerNodeId).select("svg").remove();
			if (this.legendNodeId) {
				d3.select("#"+this.legendNodeId).selectAll(".legendDiv").remove();
			}
			this.svg = this.x = this.y = this.xAxis = this.yAxis = this.xAxisG = undefined;
			this.area = this.stack = this.color = this.arc = this.pie = this.emptyPie = undefined;
			this.brush = this.brushX = this.brushY = this.brushXAxis = this.brushXAxisG = undefined;
			this.brushArea = this.brushSvg = this.brushY = this.barsGroup = undefined;
			this.yAxisLabel = this.horizontalLine = undefined;
			this.chartData = this.newChartData = this.seriesMap = this.newSeriesMap = undefined;
		},
		
		_updateXAxis: function(t, zoomed, override) {
	        console.debug(t.containerNodeId+" _updateXAxis");
			var minorTickOverride;
			if (override && override.tickValue) {
				if (override.tickUnit) {
					t.xAxis.ticks(d3.time[override.tickUnit], override.tickValue);
				} else {
					t.xAxis.ticks(override.tickValue);
				}			
				t.xAxis.tickFormat(override.tickUnit === "second" ? t._dateFormatterSeconds : t._dateFormatter);
				if (override.minorTickValue) {
					minorTickOverride = override;
				}
			} else if (zoomed) {				
				t.xAxis.ticks(t.width > 600 ? 8 : 6); // fall back on the default
				minorTickOverride = t.width > 600 ? 32 : 24;
			}
			t.svg.selectAll(".minor").remove();  // remove minor tick lines
			t.svg.select(".x.axis").call(t.xAxis); // refresh x axis
			t._createMinorTicks(t, t.xAxisG, t.x, minorTickOverride);
		},
		
		_updateYAxis: function(t, zoomed) {
			var g = t.svg.select(".y.axis");
			g.call(t.yAxis);
			// do we need to adjust spacing?
			if (t.yAxisLabel) {
				var bbox = t.yAxisLabel.node().getBBox();
				var width = bbox ? bbox.height : 32; // assume 2 lines, height because we rotated this 90 degrees
				//if (bbox) console.debug("yAxisLabel bbox.height = " + bbox.height + "; bbox.width = " + bbox.width);
				// add 30 pixels of white space
				width += 30;
				// how big is the widest tick?
				var w = 0;
				g.selectAll(".tick").each(function(d, i) {
					var b = this.getBBox();
					w = Math.max(w, b.width);
				});	
				//console.debug("widest tick is " + w);
				width = Math.floor(width+w);
				//console.debug("desired width is " + width + " while margin width is " + t.margin.left);
				if (width > t.margin.left) {
					var delta = width - t.margin.left;
					//console.debug(t.containerNodeId +": to achieve desired width, we're moving things around by " + delta);
					t.margin.left = width;
					t.width -= delta;
					// Change everything impacted by a change to left margin and width
					
					// the width of the svg container
					d3.select("#"+t.containerNodeId).select("svg")
						.attr("width", t.width + t.margin.left + t.margin.right);
					
					// Transform of the group
					t.svg.attr("transform", "translate(" + t.margin.left + "," + t.margin.top + ")");
					
					// Width of the clipPath
					t.svg.select("#"+t.containerNodeId+"_clip").select("rect")
	    				.attr("width", t.width);
					
					// x-axis range and scale
					if (t.x) {
						t.x.range([0, t.width]);
					}
					if (t.xAxis) {
						t.xAxis.scale(t.x);
					}
					
					// brush
					if (t.brushX) {
						t.brushMargin.left += delta;				
						t.brushSvg.attr("transform", "translate(" + t.brushMargin.left + "," + t.brushMargin.top + ")");
						t.brushX.range([0, t.width]);
						if (t.brushXAxisG) {
							t.brushXAxis.scale(t.brushX);
							t.brushXAxisG.select(".brushTitle")
								.attr("x", t.width/2);
						}
					}
					
					// zero line
					if (t.zeroLine) {
						t.zeroLine.attr("x2", t.width);
					}
					
					// horizontal line 
					if (t.horizontalLine) {
					    t.horizontalLine.attr("x2", t.width);
					}
					
					
					// location of the y axis label
					t.yAxisLabel.attr("y", -1*t.margin.left+20);
					
					// also change any child spans
					t.yAxisLabel.selectAll("tspan").attr("y", -1*t.margin.left+20);
					
					// overlay for mouse behaviors and related items
					var overlay = d3.select("#"+t.containerNodeId).select("svg").select(".overlayg");
					if (!overlay.empty()) {
						var overlayY = t.margin.top < 25 ? 0 : t.margin.top - 25;
						overlay.attr("transform", "translate(" + t.margin.left + "," + overlayY + ")");
					} 
					if (t.resetZoomLink) {
						t.resetZoomLink.attr("x", t.margin.left+10);
					}
					if (t.zoomBox && t.xAxisG) {
						t.xAxisG.select(".zoomTitle")
					      .attr("x", t.width/2);
					}
				}
			}
			
		},
		
		_initializeTicks: function(t, overrideSeconds) {
			if (!t.xAxis) {
				return;
			}
			if (t.tickUnit && t.tickValue) {
				t.xAxis.ticks(d3.time[t.tickUnit], t.tickValue);
			} else {
				t.xAxis.ticks(t.tickValue ? t.tickValue : 10);
			}			
			t.xAxis.tickFormat(t.tickUnit === "second" || overrideSeconds ? t._dateFormatterSeconds : t._dateFormatter);			
		},
		
		_createMinorTicks: function(t, axis, x, minorTickOverride) {
			var minorTickValue = t.minorTickValue;
			var minorTickUnit = undefined;

			if (minorTickOverride !== undefined) {
				if (typeof minorTickOverride === "number") {
					minorTickValue = minorTickOverride; 
				} else if (minorTickOverride.minorTickValue !== undefined) {
					minorTickValue = minorTickOverride.minorTickValue;
					minorTickUnit = minorTickOverride.minorTickUnit;
				}				
			}
			if (!minorTickUnit) {
				minorTickUnit = t.minorTickUnit;
			}
			
			if (minorTickUnit && minorTickValue) {
				axis.selectAll("line").data(x.ticks(d3.time[minorTickUnit], minorTickValue), function(d) { return d; })
			    .enter()
			    .append("line")
			    .attr("class", "minor")
			    .attr("y1", 0)
			    .attr("y2", 3)
			    .attr("x1", x)
			    .attr("x2", x);				
			} else if (minorTickValue) {
				axis.selectAll("line").data(x.ticks(minorTickValue), function(d) { return d; })
			    .enter()
			    .append("line")
			    .attr("class", "minor")
			    .attr("y1", 0)
			    .attr("y2", 3)
			    .attr("x1", x)
			    .attr("x2", x);								
			}
		},
		
		_wrapString: function(text, string, width, horizontal) {
			if (text.node().getComputedTextLength() <= width) {
				console.log("no wrapping needed for " + string);
				return;
			}
		    var words = string.split(/\s+/);
		    // handle the special case where we have one word that is just too long
		    if (words.length === 1) {
    			// Set a title so you can see the whole thing if you hover over it...    			
    			text.insert("svg:title", ":first-child")
    				.text(string);
    			return;
		    }
		    var line = [];
		    var lineNum = 1; // used as a multiplier for y or dy for lines 1 - n;
		    var x = text.attr("x");
		    var y = text.attr("y");
		    var yInt = parseInt(y, 10);
		    var textTruncated = false;
		    var tspan = text.text(null).append("tspan").attr("x", x).attr("y", y).attr("dy", 0);
		    for (var i=0, len=words.length; i < len; i++) {
		    	var word = words[i];
		    	line.push(word);
		    	tspan.text(line.join(" "));
		    	if (tspan.node().getComputedTextLength() > width) {
		    		// is it the only word in the line?
		    		if (line.length === 1) {
		    			tspan.text(word);
		    			textTruncated = true;
		    			// any more words?
		    			if (i === words.length-1) {
		    				// nope
		    				break;
		    			}
		    			word = null;
		    			line = [];
		    		} else {
		    			line.pop();	// remove the last word	    			
		    			tspan.text(line.join(" ")); // set the text
		    			// check for truncation
		    			if (line.length === 1 && tspan.node().getComputedTextLength() > width) {
		    				textTruncated = true;
		    			}
		    			line = [word];
		    		}
		    		// create a new tspan and increment line count
		    		if (horizontal) {
		    			tspan = text.append("tspan").attr("x", x).attr("y", yInt + lineNum * 16).text(word);	    			
		    		} else {
		    			tspan = text.append("tspan").attr("x", x).attr("y", y).attr("dy", lineNum * 16).text(word);
		    		}
		    		lineNum++;		    		
		    	}
		    }
		    if (textTruncated) {
    			// Set a title so you can see the whole thing if you hover over it...    			
    			text.insert("svg:title", ":first-child")
    				.text(string);
		    }
		},
		
		_createHorizontalLine: function(context) {
            var t = context ? context : this;
            var y= t.height + 100; // outside the clip-path
            if (t.horizontalLineOptions == undefined) {
                // add a placeholder so that the line does not appear on top of other elements
                t.horizontalLinePlaceholder = t.svg.append("line")
                    .attr("class", "horizontalLine")
                    .attr("x1", 0)
                    .attr("y1", y)
                    .attr("x2", 0)
                    .attr("y2", y)
                    .style("display", "none");
                return;
            }
            t.horizontalLine = t.horizontalLinePlaceholder;
            y = t.y(t.horizontalLineOptions.yValue);
            t.horizontalLine.style("display", null)
                .attr("x1", 0)
                .attr("y1", y)
                .attr("x2", t.width)
                .attr("y2", y)
                .attr("clip-path", "url(#"+t.containerNodeId+"_clip)")
                .style("stroke", t.horizontalLineOptions.color)
                .style("stroke-width", t.horizontalLineOptions.width);
		},
		
		_createAreaElements: function(context) {
			var t = context ? context : this;
			console.log("Creating area elements for " + t.containerNodeId);
			// create the area
			t.svg.append("path")
		      .datum(t.chartData)
		      .attr("class", "area")
		      .attr("d", t.area)
		      .attr("clip-path", "url(#"+t.containerNodeId+"_clip)")
		      .style("fill", "url(#"+getLinearGradient(t.themeColor, t.svg, t.containerNodeId)+")")
		      .style("stroke", getStrokeColor(t.themeColor))
		      .style("stroke-opacity", getOpacity(t.themeColor)); 
			
			if (t.showBrush) {
				// create the area
				t.brushSvg.append("path")
			      .datum(t.chartData)
			      .attr("class", "brushArea")
			      .attr("d", t.brushArea)
			      .style("fill", "url(#"+getLinearGradient(t.themeColor, t.svg, t.containerNodeId)+")")
			      .style("stroke", getStrokeColor(t.themeColor))
			      .style("stroke-opacity", getOpacity(t.themeColor));
								
				// add the brush
				t.brushSvg.append("g")
			      .attr("class", "x brush")
			      .call(t.brush)
			    .selectAll("rect")
			      .attr("y", -6)
			      .attr("height", t.brushHeight + 7);				
			}
		},
		
		_updateAreaElements: function(context) {
			var t = context ? context : this;
			console.log("Updating area elements for " + t.containerNodeId);

			// update the area path 
			t.svg.select("path.area")
				.datum(t.chartData)
				.attr("d", t.area);		
		
			if (t.showBrush) {
				// update the area path 
				t.brushSvg.select("path.brushArea")
					.datum(t.chartData)
					.attr("d", t.brushArea);					
			}
		},
		
		_createStackedElements: function(context) {
			var t = context ? context : this;
			console.log("Creating stacked elements for " + t.containerNodeId);
			
			// create the categories
			var category = t.svg.selectAll(".category")
					.data(t.categories)
				.enter().append("g")
					.attr("class", "category");
			
			// create the areas
			category.append("path")
		      .attr("class", "area")
		      .attr("d", function(d) { return t.area(d.values); })
		      .attr("clip-path", "url(#"+t.containerNodeId+"_clip)")
		      .style("fill", function(d) { return "url(#"+getLinearGradient(t.colors[d.name], t.svg, t.containerNodeId)+")"; })
		      .style("stroke", function(d) { return getStrokeColor(t.colors[d.name]); })
		      .style("stroke-opacity", function(d) { return getOpacity(t.colors[d.name]); })
		      .style("visibility", function(d) {
   		    	  //  handle lines for categories that aren't included...
		    	  var visibility = "visible";
		    	  if (t.seriesMap && t.seriesMap[d.name] && t.seriesMap[d.name].included === false) {
		    		  visibility = "hidden";
		    	  }
		    	  return visibility;
		      }); 
			
			if (t.showBrush) {
				// create the area
				t.brushSvg.append("path")
			      .datum(t.chartData)
			      .attr("class", "brushArea")
			      .attr("d", t.brushArea)
			      .style("fill", "url(#"+getLinearGradient("grey", t.svg, t.containerNodeId)+")")
			      .style("stroke", getStrokeColor("grey"));
				
				// add the brush
				t.brushSvg.append("g")
			      .attr("class", "x brush")
			      .call(t.brush)
			    .selectAll("rect")
			      .attr("y", -6)
			      .attr("height", t.brushHeight + 7);				
			}

		},
		
		_updateStackedElements: function(context) {
			var t = context ? context : this;
			// TODO: better way to do this?
			t.svg.selectAll(".category")
				.datum(t.categories);			
			t.svg.selectAll("path.area")
				.datum(t.categories)
				.attr("d", function(d, i) { return t.area(d[i].values); })
		        .style("visibility", function(d, i) {
	   		    	  //  handle lines for categories that aren't included...
			    	  var visibility = "visible";
			   	    if (t.seriesMap && t.seriesMap[d[i].name] && t.seriesMap[d[i].name].included === false) {
			    		  visibility = "hidden";
			    	  }
			    	  return visibility;
			      }); 

			
			if (t.showBrush) {
				// update the area path 
				t.brushSvg.select("path.brushArea")
					.datum(t.chartData)
					.attr("d", t.brushArea);		
			}
		},
		
		_createBarElements: function(context) {
			var t = context ? context : this;
			var barWidth = t.width / t.chartData.length;
			
			if (!t.barsGroup) {
				// group the bars to ensure the group is below the mouse behavior
				t.barsGroup = t.svg.append("g");
			}
			
			var bars = t.barsGroup.selectAll(".bar")
		      	.data(t.chartData);
			
			// a group for each new bar
		    var newbars = bars.enter().append("g")
		      	.attr("class", "bar")
		      	.attr("transform", function(d) { return "translate(" + t.x(d.date) + ",0)"; });
		    // the regular bar
		    newbars.append("rect")
		    	  .attr("class","bars")
			      .attr("y", function(d, i) { return t.y(d.value); })
			      .attr("height", function(d) { return t.y(0) - t.y(d.value); })
			      .attr("width", barWidth)
			      .style("fill", "url(#"+getLinearGradient(t.themeColor, t.svg, t.containerNodeId)+")")
			      .style("stroke", getStrokeColor(t.themeColor))
			      .style("stroke-opacity", getOpacity(t.themeColor));
		    // the mirror bar
		    if (t.mirror) {
			    newbars.append("rect")
			      .attr("class","mirrorbars")
			      .attr("y", function(d, i) { return t.y(0); })
			      .attr("height", function(d) { return t.y(-1*d.mirrorValue) - t.y(0); })
			      .attr("width", barWidth)
			      .style("fill", "url(#"+getLinearGradient(t.mirror.color, t.svg, t.containerNodeId)+")")
			      .style("stroke", getStrokeColor(t.mirror.color))
	              .style("stroke-opacity", getOpacity(t.mirror.color));
		    }
		    
		    bars.exit().remove();		    
		},
		
		_updateBarElements: function(context) {			
			var t = context ? context : this;
			t._createBarElements(t);

			var barWidth = t.width / t.chartData.length;
			t.barsGroup.selectAll("rect.bars")
		      	.attr("y", function(d, i) { return t.y(t.chartData[i].value); })
		      	.attr("height", function(d, i) { return t.y(0) - t.y(t.chartData[i].value); })
		      	.attr("width", barWidth);
			if (t.mirror) {
				t.barsGroup.selectAll("rect.mirrorbars")
			      	.attr("y", function(d, i) { return t.y(0); })
			      	.attr("height", function(d, i) { return t.y(-1*t.chartData[i].mirrorValue) - t.y(0); })
			      	.attr("width", barWidth);
			}
		},			
		
		_createMouseBehavior: function(context) {			
			var t = context ? context : this;
			
			var zoomBoxOutline = undefined; // svg zoom control
			var zoomStart = undefined; // start coordinates 
			var zoomEnd = undefined;   // stop coordinates
			var drawZoomBox = undefined; // function to draw the zoom box
			var zoom = undefined; // function to perform the zoom
			
			if (t.zoomBox) {
				zoomBoxOutline = t.svg.append("g")
					.attr("class", "zoomControl")
					.append("rect")
						.attr("class", "zoomBox")
						.style("display", "none")
						.attr("x", 0)
						.attr("y", 0)
						.attr("width", 0)
						.attr("height", 0);
				
				drawZoomBox = function() {
					var start = [Math.min(zoomStart[0], zoomEnd[0]), Math.min(zoomStart[1], zoomEnd[1])];
					var end = [Math.max(zoomStart[0], zoomEnd[0]), Math.max(zoomStart[1], zoomEnd[1])];		
					zoomBoxOutline.attr("x", start[0])
						.attr("y", start[1])
						.attr("width", end[0] - start[0])				
						.attr("height", end[1] - start[1])
		    	  		.style("display", null);
				};
				
				zoom = function() {					
	    	  		zoomBoxOutline.style("display", "none");
	    	  		if (zoomStart && zoomEnd) {
	    	  			// Do zoom
						var xdomainCoord = [Math.min(zoomStart[0], zoomEnd[0]), Math.max(zoomStart[0], zoomEnd[0])];
	    	  			// map the coordinates to values
					    var x = t.x.invert(xdomainCoord[0]);
				        var i = bisectDate(t.chartData, x, 1);
				        var startDate = t.chartData[i - 1].date; // err to the left
				        x = t.x.invert(xdomainCoord[1]);
				        i = bisectDate(t.chartData, x, 1);
				        if (i >= t.chartData.length) {
				        	i = t.chartData.length-1;
				        }
				        var endDate = t.chartData[i].date;
	
						var y0 = t.y.invert(zoomStart[1]);
						var y1 = t.y.invert(zoomEnd[1]);
						var startY = Math.min(y0,y1);
						var endY = Math.max(y0,y1);
						
						// clear zoom
						zoomStart = undefined;
						zoomEnd = undefined;
						
						// modify the domain based on the zoom box
						t.x.domain([startDate, endDate]);
						t.y.domain([startY, endY]);
												
						// format dates with seconds?
						t.xAxis.tickFormat(endDate.getTime() - startDate.getTime() < 600000 ? t._dateFormatterSeconds : t._dateFormatter);			
						// reduce number of y-axis ticks?
						var yRange = endY - startY;
						if (t.yAxisOptions.integers) {
							var r = Math.floor(yRange);
							r = r < 1 ? 1 : r;
							t.yAxis.ticks(Math.min(r, t.defaultYTicks));
						} else {
							t.yAxis.tickFormat(t._getFloatFormatter(yRange, 10));
						}

						// update the main chart area and axes for the new domain
						t._updateYAxis(t, true);
						t._updateXAxis(t, true);						
						if (!t.stacked) {
							t.svg.select("path.area").attr("d", t.area);
						} else {
							t.svg.selectAll("path.area")
								.attr("d", function(d, i) { return t.area(d[i].values); });
						}
						
						// show a reset link
						t.resetZoomLink.style("display", null);
	    	  		}
				};
			}

			var focus = t.svg.append("g")
		      .attr("class", "focus")
		      .style("display", "none");

			focus.append("circle")
			  .attr("r", 4.5);
			
			// add a rectangle to put a background behind the text
			var rect = focus.append("rect")
				.attr("class", "focusRect")
				.attr("x", 8)
				.attr("y", -22)
				.attr("rx", 5) // round the corners
				.attr("width", 0)
				.attr("height", 20);
	
			var text = focus.append("text")
			  .attr("x", 10)  // right 10 pixels	
			  .attr("y", -20) // above by 20 pixels
			  .attr("dy", "1em")
			  .style("text-anchor", "start");			
			
			var bisectDate = d3.bisector(function(d) { return d.date; }).left;
			var overlayY = t.margin.top < 25 ? 0 : t.margin.top - 25;
			var deltaY = t.margin.top - overlayY;
			d3.select("#"+t.containerNodeId).select("svg").append("g")
				.attr("class", "overlayg")
				.attr("transform", "translate(" + t.margin.left + "," + overlayY + ")")
			 .append("rect")
			  .attr("class", "overlay")
		      .attr("width", t.width + 10)
		      .attr("height", t.height + t.margin.top + 10)
		      .on("mousedown", function () {
		    	  	if (zoomBoxOutline) {
		    	  		console.log("mousedown - zoomStart");
		    	  		zoomStart = [d3.mouse(this)[0],d3.mouse(this)[1]-deltaY];
		    	  		d3.event.preventDefault();
		    	  	}
		    	  	return false;
		      	})
		      .on("mouseup", function () {
		    	  	if (zoomBoxOutline && zoomStart) {
		    	  		console.log("mouseup - zoomEnd");
		    	  		zoom();
		    	  		zoomStart = undefined;
		    	  		zoomEnd = undefined;
		    	  		d3.event.preventDefault();
		    	  	}
		    	  	return false;		    	  	
		      	})		      	
		      .on("mouseover", function() { focus.style("display", null); })
		      .on("mouseout", function() { 
		    	  	focus.style("display", "none"); 
		    	  	if (zoomBoxOutline && zoomStart) {
		    	  		console.log("mouseout - zoomEnd");
		    	  		zoomStart = undefined;
		    	  		zoomEnd = undefined;
		    	  		zoomBoxOutline.style("display", "none");
		    	  		d3.event.preventDefault();
		    	  	}
		    	  	return false;		    	  	
		    	  })
		      .on("mousemove", function(){		    	  
	    	  		if (zoomBoxOutline) {
	    	  			if (zoomStart) {
			    	  		console.log("mousemove - drawing zoomBox");
			    	  		focus.style("display", "none");
		    	  			zoomEnd = [d3.mouse(this)[0],d3.mouse(this)[1]-deltaY];
		    	  			drawZoomBox();
		    	  			d3.event.preventDefault();
		    	  			return false;
	    	  			} else {
	    	  				// make sure the focus is visible
	    	  				focus.style("display", null);
	    	  			}
	    	  		}	    	  		
		    	  
		    	  	var data = t.chartData;
				    var x0 = t.x.invert(d3.mouse(this)[0]);  // get the date for the y position of the mouse
			        var i = bisectDate(data, x0, 1); // find the index x0 would be inserted at in data to maintain date order
			        var d0 = data[i - 1]; // the date before the point x0 would be inserted
			        var d1 = data[i]; // the date after the point x0 would be inserted
			        var d;
			        if (!d0 ) {
			            d = d1;
			        } else if (!d1) {
			            d = d0;
			            i = i-1;
			        }
			        if (!d0 && !d1) {
                        // we have no data point to render
			            return false;
			        }
			        // Make sure it is in the current brush extent, if applicable
			        if (t.brush && !t.brush.empty()) {
			        	var brushExtent = t.brush.extent();
			        	//console.debug("brushing and mousemoved. brushExtent: " + brushExtent +", d0.date: " + d0.date +", d1.date: " + d1.date);
			        	if (d) {
			        	    if (d < brushExtent[0] || d > brushExtent[1]) {
                                // we have no data point to render, the zoom is too small
			        	        return false;
			        	    }
			        	} else {
    			        	if (d0.date < brushExtent[0]) {			        	    
    			        		if (d1.date > brushExtent[1]) {
    			        			// we have no data point to render, the zoom is too small
    			        		    //console.debug("brushing and mousemoved. zoom too small");
    			        			return false;
    			        		}
    			        		d = d1;
                                //console.debug("brushing and mousemoved. d0 date too early");
    			        	}
    			        	if (d1.date > brushExtent[1]) {
    			        		d = d0;
    			        		i = i-1;
                                //console.debug("brushing and mousemoved. d1 date too late");			        		
    			        	}
			        	}
			        	//console.debug("brushing and mousemoved. d set to " + d.date + ", i set to " + i);
			        }
			        if (!d) {
				        // pick closest date
				        if (d1 && (x0 - d0.date > d1.date - x0)) {
				        	d = d1;			        	
				        } else {
				        	d = d0;
				        	i = i-1;
				        }
			        }
			        var focusX = t.x(d.date);
                    // ensure the x coordinate is within the graph			        
			        focusX = Math.max(0, focusX);
			        focusX = Math.min(t.width, focusX);
			        
			        var hoverText = "";
			        var value = 0;

			        if (t.stacked) {
			    	  	var cats = t.categories;
				        // which category?
			    	  	var mouseOverCat = undefined; // the category the mouse is over (or closest too)
					    var mouseY = d3.mouse(this)[1]-deltaY; // y location of the mouse pointer
					    var y = 0;
					    var y0 = 0;
					    var grabFirst = false;
					    if (mouseY >= t.y(0)) {
					    	// if the  mouse is on or below the 0 axis, we want the first visible category.
					    	grabFirst = true;
					    }
				        for (var index = 0, numCats=cats.length; index < numCats; index++) {
				        	// skip any that aren't currently shown			
				        	var name = cats[index].name;
				        	if (t.seriesMap && t.seriesMap[name] && t.seriesMap[name].included === false) {
				        		continue;
				        	}
				        	mouseOverCat = name;
				        	y = cats[index].values[i].y;
				        	y0 = cats[index].values[i].y0;
				        	var ty = t.y(y0+y);
				        	var ty0 = t.y(y0);
				        	if (grabFirst || (mouseY > ty && mouseY <= ty0)) {
				        		console.log("the category is " + mouseOverCat);
				        		break;
				        	}				        	
				        } 
					    focus.attr("transform", "translate(" + focusX + "," + Math.min(Math.max(0, t.y(y0 + y)), t.height) + ")");
			        	if (t.seriesMap && t.seriesMap[mouseOverCat] && t.seriesMap[mouseOverCat].legend) {
			        		var detail = t.seriesMap[mouseOverCat].detail;
			        		hoverText = t.seriesMap[mouseOverCat].legend + (detail ? " (" + detail + ")" : "") + ": ";
			        	}
			        	value = d.value[mouseOverCat];
			        } else if (t.mirror) {
			        	// is the pointer above or below 0 axis?
			        	var cursorY = d3.mouse(this)[1]-deltaY;
			        	var yv = 0;
			        	if (cursorY > t.y(0)) {
			        		value = d.mirrorValue;
			        		yv = Math.min(t.y(-1*value), t.height);			        		
			        		hoverText = t.mirror.legend + ": ";
			        		rect.attr("y", 6);
			        		text.attr("y", 8);
			        	} else {
			        		value = d.value;
			        		yv = Math.max(0, t.y(value));
			        		hoverText = t.xAxisOptions.legend + ": ";
			        		rect.attr("y", -22);
			        		text.attr("y", -20);
			        	}
		        		focus.attr("transform", "translate(" + focusX + "," + yv + ")");
			        } else  {
			        	// ensure y not out of range due to zoom
			        	focus.attr("transform", "translate(" + focusX + "," + Math.min(Math.max(0, t.y(d.value)), t.height) + ")");
			        	value = d.value;
			        }
			        
			        if (value === null) {
			        	value = 0;
			        }
			        
		        	hoverText += t._getFloatFormatter(value, 1)(value); 

				    text.text(hoverText);	
				    var textLength = text.node().getComputedTextLength(); 
			        // pick a location for the text based on how close to the right edge it is
			        if ((textLength + focusX + 10) < t.width) {
			        	text.attr("x", 10)  // right 10 pixels	
						    .style("text-anchor", "start");
			        	rect.attr("x", 6)
			        		.attr("width", textLength+8);
			        } else {
			        	text.attr("x", -10)  // left 10 pixels	
					    	.style("text-anchor", "end");
			        	rect.attr("x", -1*(textLength+14))
		        			.attr("width", textLength+8);
			        }
			        return false;
		      	});
			
			if (t.zoomBox) {
				// create the reset zoom link and hide it
				t.resetZoomLink = d3.select("#"+t.containerNodeId).select("svg").append("text")
					.attr("y", t.margin.top > 10 ? t.margin.top - 10 : 0)
					.attr("x", t.margin.left+10)  // show on left instead of right (t.width + t.margin.left)
					.style("text-anchor", "start")  // show on left instead of right ("end")
					.style("display", "none")
					.attr("class", "resetZoom")
					.attr("cursor", "pointer")
					.text(nls.zoomControl.reset)
					.on("click", function() { dojo.publish(t.containerNodeId+"resetZoom", t); });
				}
		}
				
	});
	
	return Chart;	

});

