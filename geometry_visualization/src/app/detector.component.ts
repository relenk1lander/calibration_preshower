import { Component, OnInit } from '@angular/core';

const base = (index: number, x:number, y: number, z: number) => `
[tungsten_${index}]
type = "box"
size = 194mm 194mm 3.5mm
position = ${x}mm ${y}mm ${z}mm
orientation = 0 0 0
material = tungsten
role = "passive"

[alum_${index}]
type = "box"
size = 194mm 194mm 5mm
position = ${x}mm ${y}mm ${z + 4.25}mm
orientation = 0 0 0
material = aluminum
role = "passive"
`

const simpleBase = (index: number, subIndex: number, x: number, y: number, z: number) => `
[simple_base_alum_${index}_${subIndex}]
type = "box"
size = 132.72mm 30.56mm 3mm
position = ${x}mm ${y}mm ${z}mm
orientation = 0 0 0
material = aluminum
role = "passive"
`

const raizedBase = (index: number, subIndex: number, x: number, y: number, z: number) => `
[sec1_base_alum_${index}_${subIndex}]
type = "box"
size = 132.72mm 25.2mm 5.5mm
position = ${x}mm ${y}mm ${z}mm
orientation = 0 0 0
material = aluminum
role = "passive"

[sec2_base_alum_${index}_${subIndex}]
type = "box"
size = 132.72mm 30.56mm 1mm
position = ${x}mm ${y}mm ${z + 3.25}mm
orientation = 0 0 0
material = aluminum
role = "passive"
`

const chip = (index: [number, number], x: number, y: number, r: number, lower: boolean) => (z: number, z_index: number) => `
[Detector_${z_index}_${index[0]}_${index[1]}]
type = "faser"
position = ${x}mm ${y}mm ${z + (lower ? 9.775 : 13.275)}mm
orientation = 0deg 0deg ${r}deg
`

// 6 12
@Component({
  selector: 'app-root',
  template: `
    <svg width="663.6" height="916.8" viewBox="0 0 132720 183360" style="margin-left: 100px; border: 1px solid; display: inline-block">
      <g *ngFor="let pos of poss" attr.transform="translate({{pos.x}}, {{pos.y}})">
        <g style="opacity: 0.8" attr.transform="rotate({{pos.rotation}}, {{pos.width / 2}}, {{pos.height / 2}})">
        <rect 
        [attr.width]="pos.width" [attr.height]="pos.height"
        [attr.fill]="pos.fill" stroke="black" stroke-width="10"/>
        <rect [attr.y]="pos.height - 720"
        [attr.width]="pos.width" [attr.height]="720"/>
        </g>
      </g>
    </svg>
    <textarea id="story" name="story"
          rows="62" cols="55">
    {{ geometry }}
    </textarea>
    <input #overlap value="2680"/>
    <button (click)="calculate(overlap.value)">Calculate</button>
  `,
  styles: []
})
export class AppComponent implements OnInit {

  poss!: any[]

  geometry: string = '';

  calculate(_overlap: string | number = 2680) {

    const BASE_OFFSET = 0;

    const overlap = +_overlap;
    this.poss = [];

    
    this.geometry = '';
    let prevY = 0;
    for (let j = 0; j < 12; ++j) {
      const cond = (j % 4 === 0 && j !== 0) || (j % 4 === 2)
      const y = prevY - (cond ? overlap - BASE_OFFSET : 0) 
      for (let i = 0; i < 6; ++i) {
        this.poss.push(
          {
            i, j,
            x: i * 22120,
            y,
            width: 22120,
            height: 15280,
            rotation: j % 2 ? 0 : 180,
            fill: j % 4 < 2 ? '#F5F5DC' : 'black',
            lower: j % 4 < 2
          }
        )
      }
      prevY = y + 15280
    }

    console.log(this.poss)

    const partial_chips = this.poss.map(p => chip(
        [p.i, p.j],
        (p.x + 22120 / 2) / 1000,
        (p.y + 15280 / 2 + (p.rotation ? -260 : 260)) / 1000,
        p.rotation,
        p.lower
      )
    )

    for (let i = 0; i < 6; ++i) {
      const z = i * 30
      const x_block = 3 * 22120 / 1000
      this.geometry += base(i, (3 * 22120) / 1000, (12 * 15280 - 5 * overlap) / 2 / 1000, z)

      for (let ii = 0; ii < 3; ++ii) {
        const block_dist = 4 * 15280 - 2 * (overlap - BASE_OFFSET)
        const base_start = 15280
        const y_block = (base_start + ii * block_dist) / 1000

        const start = 3 * 15280 - (overlap - BASE_OFFSET)
        const y_block_raized = (start + ii * block_dist) / 1000

        this.geometry += simpleBase(i, ii, x_block, y_block, 8.25 + z)
        this.geometry += raizedBase(i, ii, x_block, y_block_raized, 9.5 + z)
      }

      for (const pc of partial_chips) {
        this.geometry += pc(z, i)
      }

      // break;
    }
  }

  ngOnInit(): void {
    this.calculate();
  }
}
