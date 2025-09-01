export class RocketDiagram {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext('2d');
    this.setCanvasSize();
    this.gap = 5; // Gap between sections
    this.smallGap = 2; // Smaller gap before the trapezoid
    // Default colors for each section
    this.droneColors = { background: '#ffffff', outline: '#ffffff', text: '#000000' };
    this.extensionColors = { background: '#ffffff', outline: '#ffffff', text: '#000000' };
    this.avColors = { background: '#ffffff', outline: '#ffffff', text: '#000000' };
  }

  setCanvasSize() {
    const parent = this.canvas.parentElement;
    this.canvas.width = parent.offsetWidth * 0.12; // 10% of parent's width
    this.canvas.height = parent.offsetHeight; // 100% of parent's height
    this.canvas.style.background = '#71797e'; // Dark gray background
    this.padding = 20; // Adding padding around the canvas
    this.contentHeight = this.canvas.height - 2 * this.padding; // Adjusted content height
    this.contentWidth = this.canvas.width - 2 * this.padding; // Adjusted content width
  }

  SetDroneColor(background, outline, text) {
    this.droneColors = { background, outline, text };
  }

  SetExtensionColor(background, outline, text) {
    this.extensionColors = { background, outline, text };
  }

  SetAVColor(background, outline, text) {
    this.avColors = { background, outline, text };
  }

  draw() {
    const ctx = this.ctx;
    const width = this.contentWidth;
    const height = this.contentHeight - this.gap * 3 - this.smallGap; // Adjust height to account for gaps
    const padding = this.padding;

    // Sections heights
    const triangleHeight = height * 0.20; // Taller triangle
    const textRectangleHeight = height * 0.20; // Larger rectangle for sections with text
    const emptyRectangleHeight = height * 0.15; // Smaller rectangle for sections without text
    const trapezoidHeight = height * 0.1; // Less tall trapezoid

    ctx.translate(padding, padding); // Move the context to add padding

    // Triangle (Nose Cone)
    ctx.beginPath();
    ctx.moveTo(width / 2, 0);
    ctx.lineTo(width, triangleHeight);
    ctx.lineTo(0, triangleHeight);
    ctx.closePath();
    ctx.strokeStyle = '#ffffff'; // White color for outline
    ctx.stroke();

    // Rectangles (Body Sections)
    const sections = ['Drone', 'Ext', 'AV Bay', ''];
    const colors = [this.droneColors, this.extensionColors, this.avColors, {}]; // Array of color settings
    let currentHeight = triangleHeight + this.gap;
    sections.forEach((text, index) => {
      const rectangleHeight = text ? textRectangleHeight : emptyRectangleHeight;
      const sectionColors = colors[index];
      ctx.fillStyle = sectionColors.background || '#ffffff'; // Default background if not specified
      ctx.fillRect(0, currentHeight, width, rectangleHeight);
      ctx.strokeStyle = sectionColors.outline || '#ffffff'; // Use specified outline color
      ctx.strokeRect(0, currentHeight, width, rectangleHeight);

      if (text) {
        const chars = text.split('');
        ctx.font = '16px Arial';
        ctx.fillStyle = sectionColors.text || '#000000'; // Use specified text color
        const textHeight = chars.length * 18; // Calculate total text height
        const startY = currentHeight + (rectangleHeight - textHeight) / 2 + 18; // Center text vertically
        chars.forEach((char, idx) => {
          ctx.fillText(char, (width - ctx.measureText(char).width) / 2, startY + (idx * 18)); // Center each character horizontally
        });
      }
      currentHeight += rectangleHeight + (index === sections.length - 2 ? this.smallGap : this.gap);
    });

    // Trapezoid (Booster) - flipped upside down
    ctx.beginPath();
    ctx.moveTo(0, height + 2 * this.padding);
    ctx.lineTo(width, height + 2 * this.padding);
    ctx.lineTo(width * 0.75, height + 2 * this.padding - trapezoidHeight);
    ctx.lineTo(width * 0.25, height + 2 * this.padding - trapezoidHeight);
    ctx.closePath();
    ctx.strokeStyle = '#ffffff'; // White color for outline
    ctx.stroke();

    ctx.translate(-padding, -padding); // Reset translation
  }
}

// Creating instance and drawing the diagram
const rocketDiagram = new RocketDiagram('rocket_status_diagram');
rocketDiagram.draw();
