# PROJECT CHARTER: SUPER MARIO BROS. (1985) DEVELOPMENT PROJECT

**Môn học:** Programming Systems (CS202)  
**Đội ngũ phát triển:** Nhóm 4 thành viên  
**Thời gian thực hiện:** 6 Tuần (Bao gồm thời gian dự phòng)  
**Công nghệ sử dụng:** C++17, SFML Engine, CMake Build System  

---

## 1. Tóm tắt Đề bài & Yêu cầu Dự án (Project Summary)

### 1.1. Bối cảnh & Mục tiêu

Dự án yêu cầu một đội ngũ gồm 4 nhân sự phối hợp thiết kế, xây dựng và hoàn thiện trò chơi máy tính 2D Side-scrolling Platformer mô phỏng tựa game kinh điển **Super Mario Bros. (1985)** bằng ngôn ngữ C++ và thư viện đồ họa SFML. Mục tiêu tối thượng của dự án là tối ưu hóa điểm số dựa trên thang điểm Rubric chính thức (100 điểm gốc + 15 điểm tính năng nâng cao bổ sung).

### 1.2. Thang điểm Rubric & Tiêu chí Đánh giá

Hệ thống tính năng và kiến trúc của dự án phải đáp ứng trọn vẹn các cấu phần điểm số sau:

* **Tính năng - Functionality (65 điểm):**
  * **Player Inputs, Movement and Collision (20 điểm):** Xử lý nút bấm, động lực học chuyển động vật lý (chạy, nhảy, gia tốc) và thuật toán phát hiện/giải quyết va chạm phẳng chính xác.
  * **Enemy Behavior (10 điểm):** Trí tuệ nhân tạo (AI) tuần tra và máy trạng thái xử lý hành vi của quái vật cơ bản.
  * **Power-Ups and Items (10 điểm):** Hệ thống vật phẩm tương tác đa hình làm thay đổi thuộc tính, kích thước, năng lực của nhân vật.
  * **3 Level Completion (15 điểm):** Thiết kế luồng trò chơi vượt qua tối thiểu 3 màn chơi (Levels) độc lập với độ khó tăng dần dựa trên file cấu hình.
  * **Sounds (10 điểm):** Tích hợp đầy đủ nhạc nền (Background BGM) và hiệu ứng âm thanh tương tác (SFX).
* **Thiết kế & Hiện thực - Design and Implementation (35 điểm):**
  * **Object-Oriented Design (10 điểm):** Áp dụng chuẩn mực 4 tính chất OOP (Đóng gói, Kế thừa, Đa hình, Trừu tượng).
  * **Check 5 Design Patterns (25 điểm):** Tích hợp chính xác và hiệu quả tối thiểu 5 mẫu thiết kế phần mềm để tăng tính module hóa.
* **Tính năng Nâng cao lấy Điểm cộng - Additional Requirements (15 điểm tối đa):**
  * AI cho kẻ địch/Boss nâng cao (5 điểm).
  * Hệ thống nhiều nhân vật độc lập - Multiple Player Characters (5 điểm).
  * Hiện thực hóa đồ họa game không gian 3D (5 điểm).

> 🎯 **Chiến lược Điểm số của Dự án:** Nhóm thống nhất mục tiêu đạt **110/115 điểm**. Tập trung ăn trọn 100 điểm gốc và lấy **10 điểm cộng từ AI nâng cao (Boss Màn 3) và Hệ thống đa nhân vật Mario/Luigi (Multiple Players)**. Nhóm quyết định **bỏ qua phần 3D Game** để kiểm soát rủi ro, bảo toàn chất lượng kỹ thuật tối đa cho hệ thống vật lý lưới phẳng và file lưu trữ lõi.

---

## 2. Kiến trúc Hệ thống & Ý tưởng Thiết kế (Architectural Framework)

Để đạt điểm tối đa ở phần OOP Design và 5 Design Patterns, mã nguồn của dự án được quy hoạch dựa trên cấu trúc phân cấp đa hình vững chắc và kiến trúc tách biệt thành phần lỏng (Decoupled Architecture).

### 2.1. Phân cấp Đối tượng Đa hình (Core OOP Hierarchy)

Toàn bộ thực thể trong game được quản lý tập trung bằng các mảng vector con trỏ lớp cơ sở trừu tượng nhằm đạt tính đa hình tối đa:

* `Character` (Abstract Class): Định nghĩa các thuộc tính vị trí, vận tốc, hộp va chạm và phương thức thuần ảo `update()`, `render()`.
  * `Player` (Abstract Class): Bổ sung cơ chế xử lý Input từ Command Pattern và các hàm ảo vận động.
    * `Mario`: Hiện thực thông số tốc độ, trọng lực và lực nhảy tiêu chuẩn của phiên bản 1985.
    * `Luigi`: Đa hình hóa hàm nhảy với lực đẩy trục Y lớn hơn (nhảy cao hơn), nhưng tốc độ chạy tối đa thấp hơn và quán tính trượt dài hơn do giảm hệ số ma sát mặt đất.
  * `Enemy` (Abstract Class): Định nghĩa máy trạng thái hành vi và hàm ảo `updateAI()`.
    * `Goomba`: AI tuần tra trái/phải trên lưới phẳng, tự đổi hướng khi va chạm gạch hoặc ống nước.
    * `KoopaTroopa`: Logic rụt vào vỏ khi bị dẫm lần một và biến thành vũ khí lăn gây sát thương diện rộng khi bị tác động lần hai.
    * `Bowser (Boss màn 3)`: AI nâng cao tự động nhảy bám đuổi tọa độ X của Player và khạc đạn lửa theo chu kỳ thời gian.
* `Item` (Abstract Class): Định nghĩa phương thức tương tác thuần ảo `activate(Player* player)`.
  * `Mushroom`: Kích hoạt Player chuyển đổi sang trạng thái lớn (Super Mario) thông qua việc thay đổi Bounding Box va chạm.
  * `FireFlower`: Thay đổi trạng thái Player sang Fire Mario, mở khóa kỹ năng bắn đạn cầu lửa vật lý.
  * `Coin`: Tăng điểm số trực tiếp trên thanh giao diện HUD và phát âm thanh tương tác.

### 2.2. Hiện thực hóa cụ thể 5 Design Patterns vào Hệ thống Lõi

1. **Singleton Pattern (`AssetManager`, `SoundController`):** Đảm bảo duy nhất một thực thể nắm quyền quản lý bộ đệm tài nguyên (`sf::Texture`, `sf::Font`, `sf::SoundBuffer`), giải quyết triệt để lỗi tràn bộ nhớ hoặc giật lag FPS khi tải đi tải lại file từ ổ cứng.
2. **State Pattern (`GameStateManager`):** Đóng gói và điều phối luồng chuyển cảnh mượt mà giữa các trạng thái biệt lập: `IntroMenuState`, `CharacterSelectionState` (Màn hình chọn nhân vật Mario/Luigi), `PlayState`, `PauseState`, `GameOverState`.
3. **Factory Method Pattern (`EntityFactory`):** Tự động giải mã ký tự đọc ra từ file text map (ví dụ: 'G' thành Goomba, 'M' thành Nấm) để khởi tạo động thực thể tương ứng, loại bỏ hoàn toàn việc khai báo cứng (hardcode) các lớp con.
4. **Command Pattern (`InputHandler`):** Đóng gói thao tác nhấn phím bàn phím thành các đối tượng lệnh độc lập (`JumpCommand`, `MoveCommand`). Giúp xử lý mượt mà và đồng bộ hóa việc điều khiển cho bất kỳ nhân vật nào (Mario hay Luigi) được kế thừa từ lớp `Player` mà không cần sửa đổi logic bắt phím gốc.
5. **Observer Pattern (`EventSystem`):** Tạo liên kết lỏng (Loosely Coupled). Khi Mario dẫm chết quái hoặc nhặt vật phẩm, một sự kiện phát đi, phân hệ giao diện HUD tự tăng điểm và phân hệ Sound tự phát âm thanh hiệu ứng mà không cần gọi trực tiếp mã nguồn của nhau.

### 2.3. Cơ chế Xử lý Tệp tin (File Handling & Serialization)

* **Save/Load tiến trình:** Sử dụng các luồng tệp tin `std::ofstream` và `std::ifstream` để ghi và đọc trạng thái dữ liệu người chơi (Score, Lives, Current Level, Selected Character) ra file lưu trữ `.txt` khi bấm nút Save và tải lại màn chơi tương ứng khi bấm nút Load.
* **Custom Level Editor (Tính năng thưởng):** Người chơi tự sắp xếp ô gạch/quái vật trên màn hình GUI, hệ thống tiến hành tuần tự hóa (Serialization) lưới ô vuông phẳng thành chuỗi ma trận ký tự ghi xuống file bản đồ cấu hình, sẵn sàng nạp lại thông qua `EntityFactory`.

---

## 3. Phân rã Vai trò & Lộ trình Triển khai Chi tiết

### 3.1. Ma trận phân công trách nhiệm (RACI Matrix Mapping)

| Vai trò / Nhiệm vụ | Hồng Đăng (PM) | Hải Đăng (Physics Lead) | Quốc Huy (AI & Map Lead) | Đại Nghĩa (UI & Sound) |
| :--- | :---: | :---: | :---: | :---: |
| **Kiến trúc Lõi & Quản trị** (CMake, Gitflow, Game Loop, State Pattern) | **R / A** | I | I | R |
| **Vật lý & Chuyển động** (Vật lý Euler Mario/Luigi, Va chạm AABB, Command Pattern) | C | **R / A** | C | I |
| **Quản lý Bản đồ & Thực thể** (Đọc file map, 3 Levels, Factory Method, Items) | R | C | **R / A** | I |
| **Trí tuệ Nhân tạo** (AI Tuần tra Goomba, AI Nâng cao Boss Màn 3) | I | C | **R / A** | I |
| **Giao diện, Hoạt ảnh & Hệ thống File** (Save/Load Progress, Hoạt ảnh, HUD, Observer) | **R / A** | I | R | **R / A** |

*Ghi chú: R (Responsible) - Người thực hiện; A (Accountable) - Người chịu trách nhiệm chính; C (Consulted) - Người được tham vấn; I (Informed) - Người nhận thông tin.*

### 3.2. Kế hoạch Hành động Chi tiết qua 6 Tuần

#### Tuần 1: Khởi tạo Nền tảng Kiến trúc (Design Foundations)

* **TV1 (PM):** Thiết lập CMake, cấu trúc Gitflow, file `.gitignore` chuẩn. Lập trình vòng lặp chính `Game Loop` kiểm soát Delta Time và hệ thống điều phối `GameStateManager` (**State Pattern**).
* **TV2:** Xây dựng `InputHandler` (**Command Pattern**) nhận diện và đóng gói nút bấm. Thiết kế interface trừu tượng cho lớp `Player`.
* **TV3:** Thiết lập sơ đồ cây lớp cơ sở `Character` và `Item` ứng dụng tính đóng gói và trừu tượng của OOP.
* **TV4:** Hoàn thiện bộ quản lý tập trung `AssetManager` (**Singleton Pattern**), tiến hành import dữ liệu hình ảnh và font chữ thô vào thư mục `assets/`.
* **Mục tiêu tuần:** Khởi chạy cửa sổ game thành công, chuyển đổi trơn tru từ Menu chính sang Màn hình chọn nhân vật (Character Selection Screen).

#### Tuần 2: Động lực học Chuyển động & Đọc file Màn chơi (Movement & Rendering)

* **TV1 (PM):** Viết module đọc ma trận file văn bản (.txt) chuyển đổi ký tự thành tọa độ ô lưới phẳng. Thiết lập luồng truyền trạng thái nhân vật đã chọn từ Menu vào màn chơi chính.
* **TV2:** Hiện thực thông số vật lý Euler (tính gia tốc, lực ma sát, trọng lực rơi) đa hình riêng biệt cho hai lớp con `Mario` và `Luigi`.
* **TV3:** Lập trình logic di chuyển tuần tra trái/phải cơ bản cho thực thể kẻ địch `Goomba`.
* **TV4:** Hiện thực lớp `Animation` xử lý cắt ghép sprite sheets để tạo hoạt ảnh chuyển động đồng bộ theo vận tốc vật lý thực tế của cả 2 nhân vật (Mario đỏ và Luigi xanh).
* **Mục tiêu tuần:** Chọn được nhân vật tại menu, nhân vật được chọn có hoạt ảnh chạy/nhảy bám sát chỉ số vật lý riêng biệt trên nền địa hình map tĩnh.

#### Tuần 3: Điểm nghẽn Xử lý Va chạm & Đồng bộ Thiết kế (The Core Challenge)

* **TV1 (PM):** Khởi tạo khung file xử lý tệp tin, lập trình cấu trúc lưu dữ liệu tiến trình (Score, Lives, Loại nhân vật đang chơi).
* **TV2:** **Tập trung xử lý điểm nghẽn kỹ thuật lớn nhất:** Thuật toán phát hiện và giải quyết va chạm hộp biên phẳng (**AABB Collision Resolution**) để nhân vật đứng vững trên sàn, chống lỗi lún gạch hoặc rơi xuyên bản đồ.
* **TV3:** Hiện thực hóa lớp `EntityFactory` (**Factory Method Pattern**) kết nối bộ đọc map của TV1 để tự động spawn quái vật.
* **TV4:** Xây dựng hệ thống phát thông báo sự kiện nội bộ `EventSystem` (**Observer Pattern**).
* **Mục tiêu tuần:** Vượt qua điểm nghẽn va chạm. Nhân vật đứng vững trên mặt gạch, nhảy lên húc gạch dấu hỏi gạch tự động biến đổi trạng thái thành khối rỗng.

#### Tuần 4: Luồng Gameplay Màn 1 & Camera Scrolling (Gameplay Loop & Items)

* **TV1 (PM):** Lập trình cơ chế Camera Scrolling cuộn ngang camera trục X trượt tự động theo tọa độ di chuyển của người chơi.
* **TV2:** Hiện thực va chạm chiến đấu (Mario/Luigi nhảy dẫm lên đầu quái -> quái bị bẹp; quái húc vào hông -> nhân vật bị thu nhỏ hoặc mất mạng).
* **TV3:** Hiện thực logic tương tác đa hình của các Item khi xuất hiện (Ăn nấm -> to lên; Ăn hoa lửa -> bắn đạn).
* **TV4:** Thiết kế bảng giao diện hiển thị thông số HUD (Score, Mạng sống, Thời gian), kết nối vào `EventSystem` tự tăng điểm khi ăn xu.
* **Mục tiêu tuần:** Hoàn thành trạng thái Beta hoàn chỉnh cho Màn chơi số 1 có đầy đủ luật chơi thắng/thua.

#### Tuần 5: Nhân bản Màn chơi, Hệ thống Tệp tin & Tính năng Nâng cao (Additional Features)

* **TV1 (PM):** Nhân bản và cấu hình dữ liệu văn bản để hoàn thiện trọn vẹn cấu trúc **Màn 1, Màn 2, Màn 3** tăng dần độ khó; hoàn thiện luồng ghi/đọc dữ liệu của hệ thống Save/Load game.
* **TV2:** Hỗ trợ tối ưu hóa các hàm vận động, hỗ trợ TV3 làm Custom Level Editor.
* **TV3:** Lập trình AI nâng cao cho Boss Bowser ở Màn 3 (nhảy bám đuổi, khạc đạn lửa - **5 điểm cộng**) và hoàn thiện Custom Level Editor.
* **TV4:** Tích hợp toàn bộ kho âm thanh hiệu ứng (SFX) và nhạc nền (BGM) vào các sự kiện game tương ứng.
* **Mục tiêu tuần:** Hoàn thành 100% tính năng yêu cầu trong đồ án phần mềm. Game vận hành không lỗi trên toàn bộ 3 màn chơi và lưu/tải dữ liệu ổn định.

#### Tuần 6: Tối ưu Hiệu năng, Xuất bản Tài liệu & Nghiệm thu (Deliverables)

* **Cả nhóm:** Chạy thử nghiệm liên tục (Playtesting) để tìm lỗi biên, xử lý triệt để lỗi rò rỉ bộ nhớ (Memory Leaks) bằng cách chuyển đổi sang dùng con trỏ thông minh (`std::unique_ptr` hoặc `std::shared_ptr`), giữ vững FPS ổn định ở mức 60.
* **TV1 + TV2:** Hoàn thiện sơ đồ cấu trúc lớp **Class Diagram** tổng thể thể hiện rõ tính hướng đối tượng OOP.
* **TV3 + TV4:** Hoàn thiện sơ đồ tuần tự **Sequence Diagram** minh họa cách phối hợp của các Design Patterns; quay video demo game chất lượng cao.
* **Mục tiêu tuần:** Sản phẩm đạt trạng thái hoàn hảo, đóng gói mã nguồn sạch và hồ sơ tài liệu thiết kế sẵn sàng nộp bài.

---

## 4. Quản trị Rủi ro & Quy tắc Phối hợp Nhóm (Team Policy)

1. **Kỷ luật Gitflow nghiêm ngặt:** Nghiêm cấm mọi thành viên commit code trực tiếp lên nhánh `main` hoặc `develop`. Mọi tính năng phải được phát triển trên nhánh riêng biệt (`feature/physics`, `feature/tilemap`, `feature/ui-audio`) từ nhánh `develop` đi ra và chỉ được gộp bài thông qua Pull Request sau khi Project Manager (TV1) đã kiểm duyệt cấu trúc code.
2. **Ghi dấu ấn mẫu thiết kế (Bảo hiểm điểm số):** Giảng viên chấm điểm kiến trúc dựa trên mã nguồn. Khi viết mã nguồn tại các lớp áp dụng mẫu thiết kế (như Singleton, Factory, Command, Observer, State), lập trình viên bắt buộc phải ghi chú thích bằng bình luận (comment) thật rõ ràng ở đầu tệp tin (Ví dụ: `// DESIGN PATTERN: FACTORY METHOD APPLIED FOR DYNAMIC ENTITY SPAWNING`).
3. **Tách biệt Phân hệ Lưu trữ (Decoupled File Handling):** Không viết trực tiếp các hàm ghi/đọc file tệp tin (`ifstream/ofstream`) vào bên trong các lớp nhân vật hay giao diện đồ họa. Tất cả tác vụ đọc ghi file map, lưu tiến trình phải được đóng gói gọn gàng trong module trợ giúp trung gian biệt lập để bảo toàn nguyên lý đóng gói (Encapsulation).
4. **Quản lý Bộ nhớ An toàn:** Do quản lý danh sách lớn các đối tượng `Character*` và `Item*` đa hình, việc giải phóng bộ nhớ khi thực thể bị xóa (quái vật chết hoặc item được nhặt) phải được thực hiện kỹ lưỡng bằng cách giải phóng vùng nhớ con trỏ. Nhóm khuyến khích sử dụng con trỏ thông minh `std::unique_ptr` để tránh lỗi rò rỉ bộ nhớ gây sập ứng dụng (Crash).

---

## 5. Kết luận

Tài liệu Project Charter này đóng vai trò là kim chỉ nam hành động và là cam kết kỹ thuật của toàn đội trong suốt 6 tuần phát triển dự án Super Mario Bros. (1985). Toàn bộ các thành viên có trách nhiệm tuân thủ nghiêm ngặt lộ trình thời gian, quy chuẩn mã nguồn và phân phối vai trò để hướng đến mục tiêu xuất sắc hoàn thành đồ án với điểm số tối đa.
