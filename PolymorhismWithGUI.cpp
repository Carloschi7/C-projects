//By carloschi7, for license terms read the documentation in the olcPixelGameEngine header file
// Link to the original cpp file:
// https://github.com/OneLoneCoder/olcPixelGameEngine/blob/master/Videos/OneLoneCoder_PGE_Polymorphism.cpp

/*
Requirements:
	In order to use this application, you'll need to get a copy of the olcPixelGameEngine.h
	from this link: https://github.com/OneLoneCoder/olcPixelGameEngine/blob/master/olcPixelGameEngine.h
	and save it in the project folder
Input lists :
	Pan and zoom with the left mouse button and the mouse wheel
	In order to draw a shape, select the desired one in the GUI menu and use
	the right mouse button to place each vertex.
	Key D: deletes current selected shape
	Key C: clears all of the shapes
	Key Q: allows the user to move the vertex which is being pointed by the cursor
	Key ESCAPE: Exit the application
*/

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class Node
{
private:
	olc::vi2d FromWorldToScreen(const olc::vf2d& vOffset, float fScale) const
	{
		olc::vi2d out;
		out.x = (m_Pos.x - vOffset.x) * fScale;
		out.y = (m_Pos.y - vOffset.y) * fScale;
		return out;
	}
private:
	olc::vf2d m_Pos;
public:
	Node() : Node(olc::vf2d(0.0f, 0.0f)) {}
	Node(const olc::vf2d& v) : m_Pos(v) {}
	const olc::vf2d& WorldPosition() const { return m_Pos; }
	void DrawSelf(olc::PixelGameEngine* gfx, const olc::vf2d& vOffset, float fScale) const
	{
		olc::vi2d vec = FromWorldToScreen(vOffset, fScale);
		gfx->FillCircle(vec, 2, olc::RED);
	}

	olc::vi2d ScreenPosition(const olc::vf2d& vOffset, float fScale) const 
	{
		return FromWorldToScreen(vOffset, fScale);
	}
};

class Shape
{
protected:
	uint32_t nMaxNodes;
public:
	std::vector<Node> vecNodes;
	bool bLocked = false;
public:
	Shape() : nMaxNodes(0)  {}
	Node* HitNode(const olc::vf2d& _where, uint32_t& tweak)
	{
		for (uint32_t i = 0; i < nMaxNodes; i++)
		{
			if (vecNodes[i].WorldPosition() == _where)
			{
				tweak = i;
				//This operation is allowed because we are not going to resize this vector
				//Normally returning a ptr to a vector element is a really BAD idea
				return &vecNodes[i];
			}
		}

		return nullptr;
	}

	virtual void DrawSelf(olc::PixelGameEngine* gfx, const olc::vf2d& vOffset, float fScale) = 0;
	//Will be used to implement the GUI interface
	virtual void DrawSelfStatic(olc::PixelGameEngine* gfx) = 0;
};

class Line : public Shape
{
public:
	Line()
	{
		nMaxNodes = 2;
		vecNodes.reserve(nMaxNodes);
	}

	void DrawSelf(olc::PixelGameEngine* gfx, const olc::vf2d& vOffset, float fScale) override
	{
		vecNodes[0].DrawSelf(gfx, vOffset, fScale);
		vecNodes[1].DrawSelf(gfx, vOffset, fScale);

		gfx->DrawLine(vecNodes[0].ScreenPosition(vOffset, fScale),
					  vecNodes[1].ScreenPosition(vOffset, fScale),
					  olc::WHITE);
	}

	void DrawSelfStatic(olc::PixelGameEngine* gfx) override
	{
		//Draws the shape to the screen without transformations
		//In this case, the WorldPosition method returns the non transformed
		//node position, which only for the gui figures has been set to the screen position
		gfx->FillCircle(vecNodes[0].WorldPosition(), 2, olc::RED); 
		gfx->FillCircle(vecNodes[1].WorldPosition(), 2, olc::RED);
		gfx->DrawLine(vecNodes[0].WorldPosition(), vecNodes[1].WorldPosition(), olc::WHITE);
	}
};

class Box : public Shape
{
public:
	Box()
	{
		nMaxNodes = 2;
		vecNodes.reserve(nMaxNodes);
	}

	void DrawSelf(olc::PixelGameEngine* gfx, const olc::vf2d& vOffset, float fScale) override
	{
		vecNodes[0].DrawSelf(gfx, vOffset, fScale);
		vecNodes[1].DrawSelf(gfx, vOffset, fScale);

		olc::vf2d vec0 = vecNodes[0].ScreenPosition(vOffset, fScale);
		olc::vf2d vec1 = vecNodes[1].ScreenPosition(vOffset, fScale);

		gfx->DrawRect(vec0, vec1 - vec0, olc::WHITE);
	}

	void DrawSelfStatic(olc::PixelGameEngine* gfx) override
	{
		gfx->FillCircle(vecNodes[0].WorldPosition(), 2, olc::RED);
		gfx->FillCircle(vecNodes[1].WorldPosition(), 2, olc::RED);
		
		gfx->DrawRect(vecNodes[0].WorldPosition(),
			vecNodes[1].WorldPosition() - vecNodes[0].WorldPosition(),
			olc::WHITE);
	}
};

class Circle : public Shape
{
public:
	Circle()
	{
		nMaxNodes = 2;
		vecNodes.reserve(nMaxNodes);
	}

	void DrawSelf(olc::PixelGameEngine* gfx, const olc::vf2d& vOffset, float fScale) override
	{
		vecNodes[0].DrawSelf(gfx, vOffset, fScale);
		vecNodes[1].DrawSelf(gfx, vOffset, fScale);

		olc::vf2d vec0 = vecNodes[0].ScreenPosition(vOffset, fScale);
		olc::vf2d vec1 = vecNodes[1].ScreenPosition(vOffset, fScale);

		gfx->DrawCircle(vec0, (vec1 - vec0).mag(), olc::WHITE);
	}

	void DrawSelfStatic(olc::PixelGameEngine* gfx) override
	{
		olc::vf2d vec0 = vecNodes[0].WorldPosition();
		olc::vf2d vec1 = vecNodes[1].WorldPosition();

		gfx->FillCircle(vec0, 2, olc::RED);
		gfx->FillCircle(vec1, 2, olc::RED);

		gfx->DrawCircle(vec0, (vec1 - vec0).mag(), olc::WHITE);
	}
};

class Triangle : public Shape
{
public:
	Triangle()
	{
		nMaxNodes = 3;
		vecNodes.reserve(nMaxNodes);
	}

	void DrawSelf(olc::PixelGameEngine* gfx, const olc::vf2d& vOffset, float fScale) override
	{
		vecNodes[0].DrawSelf(gfx, vOffset, fScale);
		vecNodes[1].DrawSelf(gfx, vOffset, fScale);
		vecNodes[2].DrawSelf(gfx, vOffset, fScale);

		olc::vf2d vec0 = vecNodes[0].ScreenPosition(vOffset, fScale);
		olc::vf2d vec1 = vecNodes[1].ScreenPosition(vOffset, fScale);
		olc::vf2d vec2 = vecNodes[2].ScreenPosition(vOffset, fScale);

		gfx->DrawTriangle(vec0, vec1, vec2, olc::WHITE);
	}

	void DrawSelfStatic(olc::PixelGameEngine* gfx) override
	{
		olc::vf2d vec0 = vecNodes[0].WorldPosition();
		olc::vf2d vec1 = vecNodes[1].WorldPosition();
		olc::vf2d vec2 = vecNodes[2].WorldPosition();

		gfx->FillCircle(vec0, 2, olc::RED);
		gfx->FillCircle(vec1, 2, olc::RED);
		gfx->FillCircle(vec2, 2, olc::RED);

		gfx->DrawTriangle(vec0, vec1, vec2, olc::WHITE);
	}
};

class BezierCurve : public Shape
{
public:
	BezierCurve()
	{
		nMaxNodes = 3;
		vecNodes.reserve(nMaxNodes);
	}

	void DrawSelf(olc::PixelGameEngine* gfx, const olc::vf2d& vOffset, float fScale) override
	{
		vecNodes[0].DrawSelf(gfx, vOffset, fScale);
		vecNodes[1].DrawSelf(gfx, vOffset, fScale);
		vecNodes[2].DrawSelf(gfx, vOffset, fScale);

		olc::vf2d vec0 = vecNodes[0].ScreenPosition(vOffset, fScale);
		olc::vf2d vec1 = vecNodes[1].ScreenPosition(vOffset, fScale);
		olc::vf2d vec2 = vecNodes[2].ScreenPosition(vOffset, fScale);

		gfx->DrawLine(vec0, vec1, olc::YELLOW, 0xf0f0f0f0);
		gfx->DrawLine(vec1, vec2, olc::YELLOW, 0xf0f0f0f0);

		//Draw spline
		//Bezier curve math function
		//Source:https://en.wikipedia.org/wiki/Bézier_curve
		auto Fx = [&](float t) -> olc::vf2d
		{
			return (1 - t)* (1 - t)* vec0 + 2 * t * (1 - t) * vec1 + t * t * vec2;
		};

		olc::vf2d start, end;
		for (float t = 0.0f; t < 0.99f; t += 0.01f)
		{
			start = Fx(t);
			end = Fx(t + 0.01f);
			gfx->DrawLine(start, end, olc::WHITE);
		}
	}

	void DrawSelfStatic(olc::PixelGameEngine* gfx) override
	{
		olc::vf2d vec0 = vecNodes[0].WorldPosition();
		olc::vf2d vec1 = vecNodes[1].WorldPosition();
		olc::vf2d vec2 = vecNodes[2].WorldPosition();

		gfx->FillCircle(vec0, 2, olc::RED);
		gfx->FillCircle(vec1, 2, olc::RED);
		gfx->FillCircle(vec2, 2, olc::RED);

		gfx->DrawLine(vec0, vec1, olc::YELLOW);
		gfx->DrawLine(vec1, vec2, olc::YELLOW);

		auto Fx = [&](float t) -> olc::vf2d
		{
			return (1 - t) * (1 - t) * vec0 + 2 * t * (1 - t) * vec1 + t * t * vec2;
		};

		olc::vf2d start, end;
		for (float t = 0.0f; t < 0.99f; t += 0.01f)
		{
			start = Fx(t);
			end = Fx(t + 0.01f);
			gfx->DrawLine(start, end, olc::WHITE);
		}
	}
};

class GUImenu
{
private:
	static constexpr uint32_t nItemCount = 5;

	olc::vf2d* pInitialMouse;
	int32_t m_Selected;
	olc::vf2d vCurrentPos;
	olc::vi2d vMenuSize;
	int32_t nBoxSpacing;
	olc::Pixel m_SlotColor[nItemCount];

	Line m_Line;
	Box m_Box;
	Circle m_Circle;
	Triangle m_Triangle;
	BezierCurve m_Curve;
public:
	GUImenu() : 
		pInitialMouse(nullptr),
		vCurrentPos(5.0f, 5.0f),
		vMenuSize(150, 30),
		nBoxSpacing(30),
		m_Selected(-1)
	{
		m_Line.vecNodes.resize(2);
		m_Box.vecNodes.resize(2);
		m_Circle.vecNodes.resize(2);
		m_Triangle.vecNodes.resize(3);
		m_Curve.vecNodes.resize(3);

		for (uint32_t i = 0; i < nItemCount; i++)
			m_SlotColor[i] = olc::Pixel(100, 100, 100);
	}

	inline int32_t GetSelection() const { return m_Selected; }

	void DrawSelf(olc::PixelGameEngine* gfx)
	{
		//Offset of the GUI shape figures
		m_Line.vecNodes[0] = vCurrentPos + olc::vf2d(10.0f, 10.0f);
		m_Line.vecNodes[1] = vCurrentPos + olc::vf2d(20.0f, 20.0f);

		m_Box.vecNodes[0] = vCurrentPos + olc::vf2d(10.0f + (float)nBoxSpacing, 10.0f);
		m_Box.vecNodes[1] = vCurrentPos + olc::vf2d(20.0f + (float)nBoxSpacing, 20.0f);

		m_Circle.vecNodes[0] = vCurrentPos + olc::vf2d(12.0f + (float)nBoxSpacing * 2.0f, 15.0f);
		m_Circle.vecNodes[1] = vCurrentPos + olc::vf2d(20.0f + (float)nBoxSpacing * 2.0f, 15.0f);

		m_Triangle.vecNodes[0] = vCurrentPos + olc::vf2d(10.0f + (float)nBoxSpacing * 3.0f, 20.0f);
		m_Triangle.vecNodes[1] = vCurrentPos + olc::vf2d(20.0f + (float)nBoxSpacing * 3.0f, 20.0f);
		m_Triangle.vecNodes[2] = vCurrentPos + olc::vf2d(15.0f + (float)nBoxSpacing * 3.0f, 10.0f);

		m_Curve.vecNodes[0] = vCurrentPos + olc::vf2d(10.0f + (float)nBoxSpacing * 4.0f, 20.0f);
		m_Curve.vecNodes[1] = vCurrentPos + olc::vf2d(20.0f + (float)nBoxSpacing * 4.0f, 20.0f);
		m_Curve.vecNodes[2] = vCurrentPos + olc::vf2d(15.0f + (float)nBoxSpacing * 4.0f, 10.0f);

		for (uint32_t i = 0; i < nItemCount; i++)
			gfx->FillRect(olc::vi2d(vCurrentPos.x + i * nBoxSpacing, vCurrentPos.y),
				{ nBoxSpacing, vMenuSize.y }, 
				m_SlotColor[i]);
		
		
		m_Line.DrawSelfStatic(gfx);
		m_Box.DrawSelfStatic(gfx);
		m_Circle.DrawSelfStatic(gfx);
		m_Triangle.DrawSelfStatic(gfx);
		m_Curve.DrawSelfStatic(gfx);
	}

	bool Update(olc::PixelGameEngine* gfx, float fElapsedTime)
	{
		for (uint32_t i = 0; i < nItemCount; i++)
			m_SlotColor[i] = olc::Pixel(100, 100, 100);

		olc::vf2d vMouse((float)gfx->GetMouseX(), (float)gfx->GetMouseY());
		
		//Cursor lays over the menu box
		bool bMouseInMenu = vMouse.x > vCurrentPos.x && vMouse.y > vCurrentPos.y &&
			vMouse.x < (vCurrentPos + vMenuSize).x&& vMouse.y < (vCurrentPos + vMenuSize).y;

		int32_t selected = -1;
		if (bMouseInMenu)
		{
			olc::vf2d vMouseRelative = vMouse - vCurrentPos;
			selected = (int32_t)vMouseRelative.x / nBoxSpacing;
			m_SlotColor[selected] = olc::Pixel(150, 150, 150);
		}

		if(m_Selected != -1)
			m_SlotColor[m_Selected] = olc::BLUE;

		//When this function returns true, it means that we momentarily suspend the default
		//behavior of the application(for example we suspend the panning of the plane)
		if (gfx->GetMouse(0).bPressed && bMouseInMenu)
		{
			pInitialMouse = new olc::vf2d((float)gfx->GetMouseX(), (float)gfx->GetMouseY());
			return true;
		}
		if (gfx->GetMouse(0).bHeld && pInitialMouse)
		{
			//Simple menu panning
			vCurrentPos += (vMouse - *pInitialMouse);
			*pInitialMouse = vMouse;
			return true;
		}
		if (gfx->GetMouse(0).bReleased && pInitialMouse)
		{
			if (selected != -1)
			{
				m_Selected = selected;
			}

			delete pInitialMouse;
			pInitialMouse = nullptr;
		}


		return false;
	}
};

class PolymorphismWithGUI : public olc::PixelGameEngine
{
public:
	PolymorphismWithGUI()
	{
		//Made in 12 hours as an exercise
		sAppName = "12h remake of Polymorphism proj by Carloschi7";
	}

private:
	float fScale;
	float fPointSpacing = 1.0f;
	olc::vf2d vOffset;
	olc::vf2d vStartPan;
	olc::vf2d vMouse;
	std::vector<Shape*> vecShapes;
	Shape* curShape = nullptr;
	GUImenu Menu;
public:
	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		fScale = 10.0f;
		vOffset = { (-(float)ScreenWidth() / 2.0f) / fScale, (-(float)ScreenHeight() / 2.0f) / fScale };
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		// Called once per frame, draws random coloured pixels
		if (GetKey(olc::Key::ESCAPE).bPressed) return false;
		Clear(olc::Pixel(10, 20, 89));

		//Delete everything on press
		if (GetKey(olc::C).bPressed)
		{
			for (auto& ptr : vecShapes)
				delete ptr;

			vecShapes.clear();
			curShape = nullptr;
		}

		//Delete selected shape(when is being drawn or when a vertex is geing moved)
		if (GetKey(olc::D).bPressed && curShape)
		{
			for(auto iter = vecShapes.begin(); iter != vecShapes.end(); ++iter)
				if (*iter == curShape)
				{
					vecShapes.erase(iter);
					break;
				}

			delete curShape;
			curShape = nullptr;
		}

		vMouse = { (float)GetMouseX(), (float)GetMouseY() };

		//Getting the current view on the world
		olc::vf2d vTopLeft = FromScreenToWorld({ 0,0 });
		olc::vf2d vBottomRight = FromScreenToWorld({ ScreenWidth(), ScreenHeight() });

		vTopLeft.x = std::floor(vTopLeft.x);
		vTopLeft.y = std::floor(vTopLeft.y);
		vBottomRight.x = std::ceil(vBottomRight.x);
		vBottomRight.y = std::ceil(vBottomRight.y);

		//Zoom
		olc::vf2d vStartZoom = FromScreenToWorld(vMouse);
		if (GetMouseWheel() > 0)
		{
			fScale *= 1.1f;
		}
		if (GetMouseWheel() < 0)
		{
			fScale *= 0.9f;
		}
		olc::vf2d vEndZoom = FromScreenToWorld({ GetMouseX(), GetMouseY() });
		if (vEndZoom != vStartZoom)
			vOffset -= vEndZoom - vStartZoom;

		//World cursor
		olc::vf2d vCursor = FromScreenToWorld(vMouse);
		vCursor.x = std::floor(vCursor.x + 0.5f);
		vCursor.y = std::floor(vCursor.y + 0.5f);

		if (!Menu.Update(this, fElapsedTime))
		{
			//Screen panning
			if (GetMouse(0).bPressed)
			{
				vStartPan = vMouse;
			}
			if (GetMouse(0).bHeld)
			{
				vOffset -= (vMouse - vStartPan) / fScale;
				vStartPan = vMouse;
			}

			//Determines which vertex of the shape should be moved
			static uint32_t TweakIndex = 0;
			//To prevent the user from moving a vertex when locking the shape
			bool bForbidTweak = false;

			//Ckecks if there is an actively drawn shape
			if (curShape != nullptr)
			{
				curShape->vecNodes[TweakIndex] = FromScreenToWorld(vMouse);
				if (GetMouse(1).bPressed)
				{
					//Locks the vertex
					curShape->vecNodes[TweakIndex] = vCursor;
					//If every vertex has been set, then we deselect the shape, otherwise 
					//we proceed to tweak the next vertex
					if (TweakIndex == curShape->vecNodes.size() - 1 || curShape->bLocked)
					{
						//Helps to determine whether the shape needs to be fully drawn of
						//only a single vertex is being moved with the shape already fully drawn
						curShape->bLocked = true;

						curShape = nullptr;
						bForbidTweak = true;
					}
					else
					{
						TweakIndex++;
					}
				}
			}

			//Checks if the cursor hits a previously defined point to move
			if (!curShape && GetKey(olc::Q).bHeld && !bForbidTweak)
			{
				for (auto& shape : vecShapes)
				{
					if (shape->HitNode(vCursor, TweakIndex))
					{
						curShape = shape;
						break;
					}
				}
			}

			//SHAPES
			if (!curShape && GetMouse(1).bPressed && !bForbidTweak)
			{
				switch (Menu.GetSelection())
				{
				case -1:
					break;
				case 0:
					curShape = new Line;
					vecShapes.push_back(curShape);
					curShape->vecNodes.emplace_back(vCursor);
					curShape->vecNodes.emplace_back(FromScreenToWorld(vMouse));
					TweakIndex = 1;
					break;
				case 1:
					curShape = new Box;
					vecShapes.push_back(curShape);
					curShape->vecNodes.emplace_back(vCursor);
					curShape->vecNodes.emplace_back(FromScreenToWorld(vMouse));
					TweakIndex = 1;
					break;
				case 2:
					curShape = new Circle;
					vecShapes.push_back(curShape);
					curShape->vecNodes.emplace_back(vCursor);
					curShape->vecNodes.emplace_back(FromScreenToWorld(vMouse));
					TweakIndex = 1;
					break;
				case 3:
					curShape = new Triangle;
					vecShapes.push_back(curShape);
					curShape->vecNodes.emplace_back(vCursor);
					curShape->vecNodes.emplace_back(FromScreenToWorld(vMouse));
					curShape->vecNodes.emplace_back(vCursor);
					TweakIndex = 1;
					break;
				case 4:
					curShape = new BezierCurve;
					vecShapes.push_back(curShape);
					curShape->vecNodes.emplace_back(vCursor);
					curShape->vecNodes.emplace_back(FromScreenToWorld(vMouse));
					curShape->vecNodes.emplace_back(vCursor);
					TweakIndex = 1;
					break;
				}
			}
		}

		vCursor = FromWorldToScreen(vCursor);


		//Draw axis
		olc::vf2d vStart, vEnd;
		vStart = FromWorldToScreen({ 0, vTopLeft.y });
		vEnd = FromWorldToScreen({ 0, vBottomRight.y });
		DrawLine(vStart, vEnd, olc::YELLOW);
		vStart = FromWorldToScreen({ vTopLeft.x, 0 });
		vEnd = FromWorldToScreen({ vBottomRight.x, 0 });
		DrawLine(vStart, vEnd, olc::YELLOW);

		//Draw dots
		for (float x = vTopLeft.x; x < vBottomRight.x; x += fPointSpacing)
		{
			for (float y = vTopLeft.y; y < vBottomRight.y; y += fPointSpacing)
			{
				olc::vi2d vec = FromWorldToScreen({ x,y });
				Draw(vec, olc::BLUE);
			}
		}

		//Draw Shapes
		for (const auto& ptr : vecShapes)
		{
			ptr->DrawSelf(this, vOffset, fScale);
		}

		//Draw cursor
		DrawCircle(vCursor, 2, olc::YELLOW);
		
		//Draw Menu
		Menu.DrawSelf(this);

		return true;
	}

protected:
	olc::vi2d FromWorldToScreen(const olc::vf2d& in)
	{
		olc::vi2d out;
		out.x = (in.x - vOffset.x) * fScale;
		out.y = (in.y - vOffset.y) * fScale;
		return out;
	}

	olc::vf2d FromScreenToWorld(const olc::vi2d& in)
	{
		olc::vf2d out;
		out.x = (float)in.x / fScale + vOffset.x;
		out.y = (float)in.y / fScale + vOffset.y;
		return out;
	}
};

int main()
{
	PolymorphismWithGUI demo;
	if (demo.Construct(600, 400, 2, 2))
		demo.Start();
	return 0;
}
