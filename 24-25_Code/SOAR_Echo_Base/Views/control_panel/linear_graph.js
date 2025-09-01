import '/node_modules/d3/dist/d3.js';

export class LinearGraph {
    constructor(width, height, xDomain, yDomain) {
        this.width = width;
        this.height = height;
        this.margin = { top: 20, right: 20, bottom: 30, left: 40 };
        this.xDomain = xDomain;
        this.yDomain = yDomain;
        this.data = [];

        // Initialize the scales.
        this.xScale = d3.scaleLinear()
            .domain(xDomain)
            .range([this.margin.left, width - this.margin.right]);

        this.yScale = d3.scaleLinear()
            .domain(yDomain)
            .range([height - this.margin.bottom, this.margin.top]);

        this.lineGenerator = d3.line()
            .x(d => this.xScale(d.time))
            .y(d => this.yScale(d.value));

    }

    draw() {
        // Create the SVG container.
        this.svg = d3.create("svg")
            .attr("width", this.width)
            .attr("height", this.height);

        // Add the x-axis.
        this.svg.append("g")
            .attr("transform", `translate(0,${this.height - this.margin.bottom})`)
            .attr("class", "x-axis")
            .call(d3.axisBottom(this.xScale));

        // Add the y-axis.
        this.svg.append("g")
            .attr("transform", `translate(${this.margin.left},0)`)
            .attr("class", "y-axis")
            .call(d3.axisLeft(this.yScale));

        // Create a holder for the SVG element.
        this.holder = document.createElement("div");
        this.holder.append(this.svg.node());

        return this.holder;
    }
    deleteData() {
        this.data = [];
    }
    overWriteData(newData) {
        this.data = newData;
    }
    updateChart(newData) {
        this.data.push(newData);

        const maxTime = newData.time;
        const minTime = this.data[0].time;
        if (maxTime > this.xScale.domain()[1]) {
            // Update the x-axis scale.
            this.xScale.domain([minTime-1000, maxTime+10000]);
            // Redraw the x-axis.
            this.svg.select(".x-axis")
                .call(d3.axisBottom(this.xScale));
        }
        //Find the max and min value of the data
        const maxVal = Math.max(...this.data.map(d => d.value));
        const minVal = Math.min(...this.data.map(d => d.value));
        if (maxVal > this.yScale.domain()[1] || minVal < this.yScale.domain()[0]) {
            // Update the y-axis scale.
            this.yScale.domain([minVal-100, maxVal+100]);
            // Redraw the y-axis.
            this.svg.select(".y-axis")
                .call(d3.axisLeft(this.yScale));
        }
        // Bind the new data to the circles.
        const circles = this.svg.selectAll("circle").data(this.data);

        // Enter new circles.
        circles.enter()
            .append("circle")
            .attr("cx", d => this.xScale(d.time))
            .attr("cy", d => this.yScale(d.value))
            .attr("r", 1)
            .style("fill", "white");

        // Update existing circles.
        circles
            .attr("cx", d => this.xScale(d.time))
            .attr("cy", d => this.yScale(d.value));

        // Remove old circles.
        circles.exit().remove();

        // Draw the line.
        const line = this.svg.selectAll(".line-path").data([this.data]);

        line.enter()
            .append("path")
            .attr("class", "line-path")
            .merge(line)
            .attr("d", this.lineGenerator)
            .attr("fill", "none")
            .attr("stroke", "white")
            .attr("stroke-width", 1);

        line.exit().remove();

    }
}
