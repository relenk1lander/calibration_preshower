import { Component, OnInit } from '@angular/core';

const radius = 5;
const height = radius * 2;
const width = radius * 2;

const points = [0, 1, 2, 3, 4, 5, 6].map((n, i) => {
    var angle_deg = 60 * i - 30;
    var angle_rad = Math.PI / 180 * angle_deg;
    return [
      width/2 + radius * Math.cos(angle_rad),
      height/2 + radius * Math.sin(angle_rad)
    ];
  });

const sqrt3 = Math.sqrt(3)

@Component({
  selector: 'app-pixel',
  template: `
    <svg width="2000px" height="1000px" viewBox="0 0 2000 1000">
      <circle cx="0" cy="0" r="10"/>
      <circle cx="2000" cy="1000" r="10"/>
      <polygon *ngFor="let p of points" [attr.points]="p"/>
    </svg>
  `,
  styles: [`
    polygon {
      pointer-events: visiblePainted;
      fill: hsl(60, 10%, 95%);
      stroke: hsl(0, 0%, 70%);
      stroke-width: 1px;
    }
  `]
})
export class PixelComponent implements OnInit {

  readonly points: string[] = [];

  constructor() {
    for (let j = 0; j < 128; ++j)
    for (let i = 0; i < 221; ++i) {
      const cc = sqrt3 * radius + 3
      const cur = points.map(([x, y]) => [
        x + cc * i + (j % 2 ? cc / 2 : 0),
        y + (3/2 * radius + 3)* j
      ])

      this.points.push(cur.map(arr => arr.join(',')).join(' '));
    }
  }

  ngOnInit(): void {
  }

}
