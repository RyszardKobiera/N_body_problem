#include "context_setup.h"
#include "vector"
#include <random>
#include <iostream>
#include <fstream>  
#include <math.h>
#include "implot_internal.h"


const float G = 2; // sta³a grawitacyjna
float dt = 1;

// poni¿ej s¹ warotci odpowiadaj¹ce za tworzenie uk³adu
const int number_of_planets = 200;
const float max_generating_distance = 1000; // maksymalna odleg³oæ od rodka uk³adu w ka¿dym z 3 wymiarów
const float max_generating_velocity = 3; // maksymalna prêdkoæ w ka¿dym z 3 wymiarów
const float max_generating_radius = 20; // maksymalny promieñ planet
const float max_generating_mass = 100; // maksymalna masa planet

// wartoci odpowiadaj¹ce za wywietlanie
const int number_of_segments = 20; // iloæ cian wywietlanych planet
ImVec2 plot_size = ImVec2(1000, 1000); // rozmiar wywietlanego wykresu



float GetAxesRange(ImAxis ax) { // funkcja zwracaj¹ca skalê na wykresie
    ImPlotContext& gp = *GImPlot;
    ImPlotPlot& plot = *gp.CurrentPlot;
    ImPlotAxis& axis = plot.Axes[ax];
    return fabs(axis.ScaleMin - axis.ScaleMax);
}

class Planet { // klasa przechowywuj¹ca parametry pojedyñczego obiektu w ukladzie
public:
    float x = 0;
    float y = 0;
    float z = 0;

    float vx = 0;
    float vy = 0;
    float vz = 0;

    float m = 0;
    float r = 0;

    std::vector<float> acceleration_from_object(float obj_x, float obj_y, float obj_z, float obj_m) { // funkcja na podstawie parametrów drugiej planety oblicza z jakie przyspieszenie ta druga planeta nada pierwszej
        std::vector<float> acceleration; // wynik jest zwracany w formie 3 wymiarowego wektora
        acceleration.resize(3);
        float distance_x = obj_x - x;
        float distance_y = obj_y - y;
        float distance_z = obj_z - z;
        float distance = sqrt(distance_x * distance_x + distance_y * distance_y + distance_z * distance_z);
        float ac = G * obj_m / (distance * distance);
        acceleration[0] = ac * distance_x / distance;
        acceleration[1] = ac * distance_y / distance;
        acceleration[2] = ac * distance_z / distance;
        return acceleration;
    }
    Planet() {

    }
    void merge(Planet a) { // funkcja nadaj¹ca planecie odpowiednie parametry po zderzeniu z inn¹ planet¹ podawan¹ jako argument
        x = (x * m + a.x * a.m) / (m + a.m);
        y = (y * m + a.y * a.m) / (m + a.m);
        z = (z * m + a.z * a.m) / (m + a.m);
        vx = (vx * m + a.vx * a.m) / (m + a.m);
        vy = (vy * m + a.vy * a.m) / (m + a.m);
        vz = (vz * m + a.vz * a.m) / (m + a.m);
        m += a.m;

        r = pow(pow(r, 3) + pow(a.r, 3), 1.0 / 3.0);
    }
};

class Trajectory { //klasa przechowuj¹ca trajektorie planety oraz posiadj¹ca jej index w wektorze systemu, który posiada wszystkie planety
public:
    int planet_index = 0;
    std::vector<std::vector<float>> points;

    Trajectory(int i = 0) {
        int planet_index = i;
    }
    void add_point(float x, float y, float z) { //metoda dodaj¹ca kolejny punkt do trajektori
        std::vector<float> point;
        point.resize(3);
        point[0] = x;
        point[1] = y;
        point[2] = z;
        points.push_back(point);
    }
    ImVector<ImPlotPoint> generate_points(float teta, float fi) { //metoda generuj¹ca na podstawie 3 wymiarowej trajektori ImVector dwuwymiarowych punktów do wywietlenia na wykresie tak aby przedstawia³y one trajektoriê
        // funkcja przyjmuje k¹ty pod jakimi patrzymy na uklad w radianach
        float x1, y1, z2 = 0;
        ImPlotPoint plot_point;
        ImVector<ImPlotPoint> plot_points;
        for (int i = 0; i < points.size(); i++) {//iteracja po wszystkich 3 wymiarowych punktach trajektoeii aby dla ka¿dego wygenerowac punkt rzutowania

            // poni¿ej jest obrot wokó³ osi z o fi
            // indeksy punktów 0,1,2 to odpowiednio po³o¿enie x,y,z
            x1 = cos(fi) * points[i][0] + sin(fi) * points[i][1];
            y1 = cos(fi) * points[i][1] + sin(fi) * points[i][0];

            // poni¿ej obrót wokó³ starej osi x o teta
            z2 = cos(teta) * points[i][2] + sin(teta) * y1;
            plot_point.x = x1;
            plot_point.y = z2;
            plot_points.push_back(plot_point);
        }
        return plot_points;
    }


};

class System { // klasa przechowuj¹ca wszytkie parametry uk³adu
public:
    std::vector<Planet> planets;

    Trajectory trajectories[5];

    System() {
        for (int i = 0; i < 5; i++) { // przypisuje trajektoriom 5 pierwszych planet w indeksie chyba jesli jest co najmniej 5 planet w uk³adzie
            if (i < number_of_planets) {
                trajectories[i].planet_index = i;
            }
            else {
                trajectories[i].planet_index = 0;
            }

        }
    }

    void add_planet(Planet a) {
        planets.push_back(a);
    }

    ImVector<ImPlotPoint> generate_points(float teta, float fi) {// metoda generuje rzut pozycji wszystkich planet na zadan¹ k¹tami p³aszczyznê
        // wartoci s¹ zwracane w formie ImVectora
        float x1, y1, z2 = 0;
        ImPlotPoint point;
        ImVector<ImPlotPoint> points;
        for (int i = 0; i < planets.size(); i++) {
            x1 = cos(fi) * planets[i].x + sin(fi) * planets[i].y;
            y1 = cos(fi) * planets[i].y + sin(fi) * planets[i].x;
            z2 = cos(teta) * planets[i].z + sin(teta) * y1;
            point.x = x1;
            point.y = z2;
            points.push_back(point);
        }
        return points;
    }

    void sort(float teta, float fi) {// aby wywietlaæ planety znajduj¹ce siê "dalej" na osi y za planetami "bli¿ej" potrzebna jest funkcja, która u³o¿y je w opowiedniej kolejnoci
        float x1, y1, y2 = 0;
        std::vector<float> ys;
        for (int i = 0; i < planets.size(); i++) {// pêtla tworz¹ca wektor z wszystki mi wartociami glêboci dla danego k¹ta patrzenia
            x1 = cos(fi) * planets[i].x + sin(fi) * planets[i].y;
            y1 = cos(fi) * planets[i].y + sin(fi) * planets[i].x;
            y2 = sin(teta) * planets[i].z + cos(teta) * y1;
            ys.push_back(y2);
        }
        float y = 0;
        Planet a;
        for (int i = 0; i < ys.size(); i++) {// pêtla sortuj¹ca jednoczenie wartoci g³êbokoci i planety
            for (int j = 0; j < ys.size() - i - 1; j++) {
                if (ys[j] > ys[j + 1]) {
                    y = ys[j];
                    ys[j] = ys[j + 1];
                    ys[j + 1] = y;
                    a = planets[j];
                    planets[j] = planets[j + 1];
                    planets[j + 1] = a;
                    for (int k = 0; k < 5; k++) { // pêtla zmieniaj¹ca indeksy planet w trajektoriach jeli ledzona planeta zmieni miejsce w wektorze podczas sortowania
                        if (trajectories[k].planet_index == j) {
                            trajectories[k].planet_index = j + 1;
                        }
                        else if (trajectories[k].planet_index == j + 1) {
                            trajectories[k].planet_index = j;
                        }
                    }
                }
            }
        }
    }


    float distance(Planet a, Planet b) {
        return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
    }

    std::vector<float> acceleration(int planet_index) {// metoda obliczaj¹ca przyspieszenie jakie nada ca³y uk³ad danej planecie
        // wynik jest zwracany w formie 3 wymiarowego wektora
        std::vector<float> acceleration;
        acceleration.resize(3);
        std::vector<float> new_acceleration;
        new_acceleration.resize(3);
        for (int i = 0; i < 3; i++) {
            acceleration[i] = 0;
        }
        for (int i = 0; i < planets.size(); i++) {
            if (planet_index != i) {
                new_acceleration = planets[planet_index].acceleration_from_object(planets[i].x, planets[i].y, planets[i].z, planets[i].m);
                for (int i = 0; i < 3; i++) {
                    acceleration[i] += new_acceleration[i];
                }
            }
        }
        return acceleration;
    }


    void update() {// funkcja wykonuj¹ca jeden krok
        System new_system;
        for (int i = 0; i < planets.size(); i++) {
            Planet new_planet;
            new_planet.m = planets[i].m;
            new_planet.r = planets[i].r;
            std::vector<float> planet_acceleration;
            planet_acceleration.resize(3);
            planet_acceleration = acceleration(i);

            //generowanie nowyh predkoci v(t+dt/2) przy za³o¿eniu ¿e prêdkoci z poprzedniego kroku to v(t-dt/2)
            new_planet.vx = planets[i].vx + planet_acceleration[0] * dt;
            new_planet.vy = planets[i].vy + planet_acceleration[1] * dt;
            new_planet.vz = planets[i].vz + planet_acceleration[2] * dt;

            //generowanie nowych po³o¿eñ z wzoru x(t+dt) = x(t) + dt*v(t+dt/2) 
            new_planet.x = planets[i].x + new_planet.vx * dt;
            new_planet.y = planets[i].y + new_planet.vy * dt;
            new_planet.z = planets[i].z + new_planet.vz * dt;
            new_system.add_planet(new_planet);
        }

        for (int i = 0; i < new_system.planets.size(); i++) {//pêtla sprawdzaj¹ca kolizje
            for (int j = i + 1; j < new_system.planets.size(); j++) {
                if (distance(new_system.planets[i], new_system.planets[j]) < new_system.planets[i].r + new_system.planets[j].r) {
                    for (int k = 0; k < 5; k++) {// pêtla zmieniaj¹ca index planet sledzonych przez 5 trajektorii w wypadku zderzenia
                        if (trajectories[k].planet_index == j) {
                            trajectories[k].planet_index = i;
                        }
                        else if (trajectories[k].planet_index > j) {
                            trajectories[k].planet_index -= 1;
                        }
                    }
                    new_system.planets[i].merge(new_system.planets[j]);
                    new_system.planets.erase(new_system.planets.begin() + j);
                    j--;
                }
            }
        }

        planets = new_system.planets;
        for (int i = 0; i < 5; i++) {// dodawanie kolejnego punktu do trajektorii
            trajectories[i].add_point(planets[trajectories[i].planet_index].x, planets[trajectories[i].planet_index].y, planets[trajectories[i].planet_index].z);
        }

    }


};


void create_file(int number_of_objects, std::string file_name) {// funkcja tworz¹ca plik tekstowy z losowo wygenorowanymi parametrami pocz¹tkowymi
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist1(-max_generating_distance, max_generating_distance);
    std::uniform_real_distribution<float> dist2(-max_generating_velocity, max_generating_velocity);
    std::uniform_real_distribution<float> dist3(0, max_generating_radius);
    std::uniform_real_distribution<float> dist4(0, max_generating_mass);
    std::ofstream file(file_name);

    for (int i = 0; i < number_of_objects; i++) {
        file << dist1(mt) << " " << dist1(mt) << " " << dist1(mt) << " " << dist2(mt) << " " << dist2(mt) << " " << dist2(mt) << " " << dist4(mt) << " " << dist3(mt) << "\n";
    }
    file.close();
}

System read_file(std::string file_name) {// funkcja tworz¹ca system na podstawie parametrów pocz¹tkowych
    System parameters;
    std::ifstream file(file_name);
    float x, y, z, vx, vy, vz, m, r = 0;
    int i = 0;
    while (file >> x >> y >> z >> vx >> vy >> vz >> m >> r) {
        Planet new_planet;
        new_planet.x = x;
        new_planet.y = y;
        new_planet.z = z;
        new_planet.vx = vx;
        new_planet.vy = vy;
        new_planet.vz = vz;
        new_planet.m = m;
        new_planet.r = r;


        parameters.add_planet(new_planet);
    }
    return parameters;
}




int main(int, char**)
{	
	
	

    ImGuiContext_OGL context;
    context.setup_and_show();

    //k¹ty pod jakimi mo¿na patrzec na uk³ad
    float teta = 0;
    float fi = 0;

    std::string file_name = "parametry.txt"; // nazwa pliku z parametrami pocz¹tkowymi
    create_file(number_of_planets, file_name);

    System a;

    a = read_file(file_name);



    //tworzenie zmiennych w których przechowywane s¹ dane do wyswietlenia
    ImVector<ImPlotPoint> points; //po³o¿enie planet
    std::vector<ImVector<ImPlotPoint>> lines; // 5 trajektorii
    lines.resize(5);


	

	
	
    while (!glfwWindowShouldClose(context.window))
    {
		
        a.update();//wykonanie kroku
       

        context.new_frame();

        ImGui::Begin("First window");
        ImGui::Text("Obrot wokol dwoch osi:");

        ImGui::SliderFloat("fi", &fi, 0, 6.3);
        ImGui::SliderFloat("teta", &teta, 0, 3.14);
        ImGui::SliderFloat("dt", &dt, 0, 10);
       

        a.sort(teta, fi);//sortuje punkty na podstawie odleg³oci od p³aszczyzny rzutowania

        points = a.generate_points(teta, fi);//tworzy rzuty planet na p³aszczyznê
        for (int i = 0; i < 5; i++) {
            lines[i] = a.trajectories[i].generate_points(teta, fi); // tworzy rzuty trajektorii na p³aszczyznê
        }


        ImPlot::BeginPlot("My_Plot", plot_size);
        
        ImPlot::PushPlotClipRect();
        
        float scale = GetAxesRange(ImAxis_X1);
        
        
        
        for (int i = 0; i < a.planets.size(); i++) {//pêtla wyswietlaj¹ca planety
            ImVec2 p_0 = ImPlot::PlotToPixels(points[i]);
            ImPlot::GetPlotDrawList()->AddCircleFilled(p_0, a.planets[i].r * 700 / scale, IM_COL32(255, abs(log(a.planets[i].m) * 25), 0, 255), number_of_segments);
            //nie wiem dlaczego ale metoda opisana w instrukcji z dzieleniem przez skalê tworzy³a punkty zdecydowanie za ma³e
            // metod¹ prób i b³êdów oceni³em ¿e powiêkszenie ich ok 700 krotnie sprawi, ¿e ich wywietlana wielkoæ bêdzie siê pokrywa³a z faktyczn¹
        }
        

        for (int i = 0; i < 5; i++) {// pêtla wywietlaj¹ca trajektorie
           ImPlot::PlotLine(" ", &lines[i][0].x, &lines[i][0].y, lines[i].size(), 0, 0, 2 * sizeof(double));

        }
		ImPlot::PopPlotClipRect();
        ImPlot::EndPlot();


        ImGui::End();

        context.render_frame();
    }

    return 0;
}
