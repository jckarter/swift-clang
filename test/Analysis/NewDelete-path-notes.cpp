// RUN: %clang_cc1 -analyze -analyzer-checker=cplusplus.NewDelete -analyzer-output=text -verify %s
// RUN: %clang_cc1 -analyze -analyzer-checker=cplusplus.NewDelete -analyzer-output=plist %s -o %t.plist
// RUN: FileCheck --input-file=%t.plist %s

void test() {
  int *p = new int;
  // expected-note@-1 {{Memory is allocated}}
  if (p)
    // expected-note@-1 {{Assuming 'p' is non-null}}
    // expected-note@-2 {{Taking true branch}}
    delete p;
    // expected-note@-1 {{Memory is released}}

  delete p; // expected-warning {{Attempt to free released memory}}
  // expected-note@-1 {{Attempt to free released memory}}
}

// CHECK:  <key>diagnostics</key>
// CHECK-NEXT:<array>
// CHECK-NEXT: <dict>
// CHECK-NEXT:  <key>path</key>
// CHECK-NEXT:  <array>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>control</string>
// CHECK-NEXT:    <key>edges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:      <dict>
// CHECK-NEXT:       <key>start</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>6</integer>
// CHECK-NEXT:          <key>col</key><integer>3</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>6</integer>
// CHECK-NEXT:          <key>col</key><integer>5</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:       <key>end</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>6</integer>
// CHECK-NEXT:          <key>col</key><integer>12</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>6</integer>
// CHECK-NEXT:          <key>col</key><integer>14</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:      </dict>
// CHECK-NEXT:     </array>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>event</string>
// CHECK-NEXT:    <key>location</key>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>line</key><integer>6</integer>
// CHECK-NEXT:     <key>col</key><integer>12</integer>
// CHECK-NEXT:     <key>file</key><integer>0</integer>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <key>ranges</key>
// CHECK-NEXT:    <array>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>line</key><integer>6</integer>
// CHECK-NEXT:        <key>col</key><integer>12</integer>
// CHECK-NEXT:        <key>file</key><integer>0</integer>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>line</key><integer>6</integer>
// CHECK-NEXT:        <key>col</key><integer>18</integer>
// CHECK-NEXT:        <key>file</key><integer>0</integer>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </array>
// CHECK-NEXT:    <key>depth</key><integer>0</integer>
// CHECK-NEXT:    <key>extended_message</key>
// CHECK-NEXT:    <string>Memory is allocated</string>
// CHECK-NEXT:    <key>message</key>
// CHECK-NEXT:    <string>Memory is allocated</string>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>control</string>
// CHECK-NEXT:    <key>edges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:      <dict>
// CHECK-NEXT:       <key>start</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>6</integer>
// CHECK-NEXT:          <key>col</key><integer>12</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>6</integer>
// CHECK-NEXT:          <key>col</key><integer>14</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:       <key>end</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>8</integer>
// CHECK-NEXT:          <key>col</key><integer>3</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>8</integer>
// CHECK-NEXT:          <key>col</key><integer>4</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:      </dict>
// CHECK-NEXT:     </array>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>control</string>
// CHECK-NEXT:    <key>edges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:      <dict>
// CHECK-NEXT:       <key>start</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>8</integer>
// CHECK-NEXT:          <key>col</key><integer>3</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>8</integer>
// CHECK-NEXT:          <key>col</key><integer>4</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:       <key>end</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>8</integer>
// CHECK-NEXT:          <key>col</key><integer>7</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>8</integer>
// CHECK-NEXT:          <key>col</key><integer>7</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:      </dict>
// CHECK-NEXT:     </array>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>event</string>
// CHECK-NEXT:    <key>location</key>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>line</key><integer>8</integer>
// CHECK-NEXT:     <key>col</key><integer>7</integer>
// CHECK-NEXT:     <key>file</key><integer>0</integer>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <key>ranges</key>
// CHECK-NEXT:    <array>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>line</key><integer>8</integer>
// CHECK-NEXT:        <key>col</key><integer>7</integer>
// CHECK-NEXT:        <key>file</key><integer>0</integer>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>line</key><integer>8</integer>
// CHECK-NEXT:        <key>col</key><integer>7</integer>
// CHECK-NEXT:        <key>file</key><integer>0</integer>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </array>
// CHECK-NEXT:    <key>depth</key><integer>0</integer>
// CHECK-NEXT:    <key>extended_message</key>
// CHECK-NEXT:    <string>Assuming &apos;p&apos; is non-null</string>
// CHECK-NEXT:    <key>message</key>
// CHECK-NEXT:    <string>Assuming &apos;p&apos; is non-null</string>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>control</string>
// CHECK-NEXT:    <key>edges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:      <dict>
// CHECK-NEXT:       <key>start</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>8</integer>
// CHECK-NEXT:          <key>col</key><integer>7</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>8</integer>
// CHECK-NEXT:          <key>col</key><integer>7</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:       <key>end</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>11</integer>
// CHECK-NEXT:          <key>col</key><integer>5</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>11</integer>
// CHECK-NEXT:          <key>col</key><integer>10</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:      </dict>
// CHECK-NEXT:     </array>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>event</string>
// CHECK-NEXT:    <key>location</key>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>line</key><integer>11</integer>
// CHECK-NEXT:     <key>col</key><integer>5</integer>
// CHECK-NEXT:     <key>file</key><integer>0</integer>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <key>ranges</key>
// CHECK-NEXT:    <array>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>line</key><integer>11</integer>
// CHECK-NEXT:        <key>col</key><integer>5</integer>
// CHECK-NEXT:        <key>file</key><integer>0</integer>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>line</key><integer>11</integer>
// CHECK-NEXT:        <key>col</key><integer>12</integer>
// CHECK-NEXT:        <key>file</key><integer>0</integer>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </array>
// CHECK-NEXT:    <key>depth</key><integer>0</integer>
// CHECK-NEXT:    <key>extended_message</key>
// CHECK-NEXT:    <string>Memory is released</string>
// CHECK-NEXT:    <key>message</key>
// CHECK-NEXT:    <string>Memory is released</string>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>control</string>
// CHECK-NEXT:    <key>edges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:      <dict>
// CHECK-NEXT:       <key>start</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>11</integer>
// CHECK-NEXT:          <key>col</key><integer>5</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>11</integer>
// CHECK-NEXT:          <key>col</key><integer>10</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:       <key>end</key>
// CHECK-NEXT:        <array>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>14</integer>
// CHECK-NEXT:          <key>col</key><integer>3</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:         <dict>
// CHECK-NEXT:          <key>line</key><integer>14</integer>
// CHECK-NEXT:          <key>col</key><integer>8</integer>
// CHECK-NEXT:          <key>file</key><integer>0</integer>
// CHECK-NEXT:         </dict>
// CHECK-NEXT:        </array>
// CHECK-NEXT:      </dict>
// CHECK-NEXT:     </array>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:   <dict>
// CHECK-NEXT:    <key>kind</key><string>event</string>
// CHECK-NEXT:    <key>location</key>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>line</key><integer>14</integer>
// CHECK-NEXT:     <key>col</key><integer>3</integer>
// CHECK-NEXT:     <key>file</key><integer>0</integer>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <key>ranges</key>
// CHECK-NEXT:    <array>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>line</key><integer>14</integer>
// CHECK-NEXT:        <key>col</key><integer>3</integer>
// CHECK-NEXT:        <key>file</key><integer>0</integer>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>line</key><integer>14</integer>
// CHECK-NEXT:        <key>col</key><integer>10</integer>
// CHECK-NEXT:        <key>file</key><integer>0</integer>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </array>
// CHECK-NEXT:    <key>depth</key><integer>0</integer>
// CHECK-NEXT:    <key>extended_message</key>
// CHECK-NEXT:    <string>Attempt to free released memory</string>
// CHECK-NEXT:    <key>message</key>
// CHECK-NEXT:    <string>Attempt to free released memory</string>
// CHECK-NEXT:   </dict>
// CHECK-NEXT:  </array>
// CHECK-NEXT:  <key>description</key><string>Attempt to free released memory</string>
// CHECK-NEXT:  <key>category</key><string>Memory Error</string>
// CHECK-NEXT:  <key>type</key><string>Double free</string>
// CHECK-NEXT: <key>issue_context_kind</key><string>function</string>
// CHECK-NEXT: <key>issue_context</key><string>test</string>
// CHECK-NEXT: <key>issue_hash</key><string>9</string>
// CHECK-NEXT: <key>location</key>
// CHECK-NEXT: <dict>
// CHECK-NEXT:  <key>line</key><integer>14</integer>
// CHECK-NEXT:  <key>col</key><integer>3</integer>
// CHECK-NEXT:  <key>file</key><integer>0</integer>
// CHECK-NEXT: </dict>
// CHECK-NEXT: </dict>
// CHECK-NEXT:</array>
